#!/bin/bash

# readlink -f cannot work on mac
TOPDIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

BUILD_SH=$TOPDIR/build.sh

CMAKE_COMMAND="cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 --log-level=STATUS"

ALL_ARGS=("$@")
BUILD_ARGS=()
MAKE_ARGS=()
MAKE=make

echo "$0 ${ALL_ARGS[@]}"

function usage
{
  echo "Usage:"
  echo "./build.sh -h"
  echo "./build.sh init # install dependence"
  echo "./build.sh clean"
  echo "./build.sh make"      #添加常用命令
  echo "./build.sh unittest"
  echo "./build.sh test [TestCases] [TestOptions]"
  echo "./build.sh dif SingleTestCaseName [DiffOptions]"
  echo "./build.sh [BuildType] [--make [MakeOptions]]"
  echo ""
  echo "OPTIONS:"
  echo "BuildType => debug(default), release"
  echo "MakeOptions => Options to make command, default: -j N"
  echo "DiffOptions => Options to diff command, e.g. -u|-c|-y"  # -u: unified, -c: context, -y: side by side
  echo "TestOptions => Options to 。/test/case/miniob_test.py" 

  echo ""
  echo "Examples:"
  echo "# Init."
  echo "./build.sh init"
  echo ""
  echo "# Build by debug mode and make with -j24."
  echo "./build.sh debug --make -j24"
  echo ""
  echo "./build.sh make"
  echo ""
  echo "./build.sh test"
  echo "./build.sh unittest"
  echo "./build.sh test primary-drop-table,basic"
  echo ""
  echo "./build.sh diff primary-drop-table -u"
}

function parse_args
{
  make_start=false
  for arg in "${ALL_ARGS[@]}"; do
    if [[ "$arg" == "--make" ]]
    then
      make_start=true
    elif [[ $make_start == false ]]
    then
      BUILD_ARGS+=("$arg")
    else
      MAKE_ARGS+=("$arg")
    fi

  done
}

# try call command make, if use give --make in command line.
function try_make
{
  if [[ $MAKE != false ]]
  then
    # use single thread `make` if concurrent building failed
    $MAKE "${MAKE_ARGS[@]}" || $MAKE
  fi
}

function clang_format
{
  if [[ $MAKE != false ]]
  then
    $MAKE format
  fi
}

function check_clang_tidy
{
  if [[ $MAKE != false ]]
  then
    $MAKE check-clang-tidy
  fi
}

# create build directory and cd it.
function prepare_build_dir
{
  TYPE=$1
  mkdir -p $TOPDIR/build_$TYPE && cd $TOPDIR/build_$TYPE
}

function do_init
{
  git submodule update --init || return
  current_dir=$PWD

  MAKE_COMMAND="make --silent"

  # build libevent
  cd ${TOPDIR}/deps/3rd/libevent && \
    git checkout release-2.1.12-stable && \  #不知道干了什么
    mkdir -p build && \
    cd build && \
    ${CMAKE_COMMAND} .. -DEVENT__DISABLE_OPENSSL=ON -DEVENT__LIBRARY_TYPE=BOTH && \
    ${MAKE_COMMAND} -j4 && \
    make install

  # build googletest
  cd ${TOPDIR}/deps/3rd/googletest && \
    mkdir -p build && \
    cd build && \
    ${CMAKE_COMMAND} .. && \
    ${MAKE_COMMAND} -j4 && \
    ${MAKE_COMMAND} install

  # build google benchmark
  cd ${TOPDIR}/deps/3rd/benchmark && \
    mkdir -p build && \
    cd build && \
    ${CMAKE_COMMAND} .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBENCHMARK_ENABLE_TESTING=OFF  -DBENCHMARK_INSTALL_DOCS=OFF -DBENCHMARK_ENABLE_GTEST_TESTS=OFF -DBENCHMARK_USE_BUNDLED_GTEST=OFF -DBENCHMARK_ENABLE_ASSEMBLY_TESTS=OFF && \
    ${MAKE_COMMAND} -j4 && \
    ${MAKE_COMMAND} install

  # build jsoncpp
  cd ${TOPDIR}/deps/3rd/jsoncpp && \
    mkdir -p build && \
    cd build && \
    ${CMAKE_COMMAND} -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF .. && \
    ${MAKE_COMMAND} && \
    ${MAKE_COMMAND} install

  cd $current_dir
}

function prepare_build_dir
{
  TYPE=$1
  mkdir -p ${TOPDIR}/build_${TYPE}
  rm -f build
  echo "create soft link for build_${TYPE}, linked by directory named build"
  ln -s build_${TYPE} build
  cd ${TOPDIR}/build_${TYPE}
}

function do_build
{
  TYPE=$1; shift
  prepare_build_dir $TYPE || return
  echo "${CMAKE_COMMAND} ${TOPDIR} $@"
  ${CMAKE_COMMAND} -S ${TOPDIR} $@
}

function do_clean
{
  echo "clean build_* dirs"
  find . -maxdepth 1 -type d -name 'build_*' | xargs rm -rf
}

function build
{
  set -- "${BUILD_ARGS[@]}"
  case "x$1" in
    xrelease)
      do_build "$@" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDEBUG=OFF
      ;;
    xdebug)
      do_build "$@" -DCMAKE_BUILD_TYPE=Debug -DDEBUG=ON
      ;;
    *)
      BUILD_ARGS=(debug "${BUILD_ARGS[@]}")
      build
      ;;
  esac
}

function do_test
{
  # test component
  # pushd build/bin
  # for exe in $(pwd)/*_test; do
  #   if [ $exe != $(pwd)/"client_performance_test" ] && [ $exe != $(pwd)/"clog_test" ]; then
  #     $exe
  #   fi
  # done
  # popd
  set -x
  if [[ $# > 1 ]]; then
    python3 ./test/case/miniob_test.py --test-cases=${@:2}
  else
    # all cases
    python3 ./test/case/miniob_test.py
  fi
}

function do_diff
{
  # usage:
  # ./diff_single_case.sh basic    # default format
  # ./diff_single_case.sh basic -u # unified format
  # ./diff_single_case.sh basic -c # context format
  # ./diff_single_case.sh basic -y # two column format
  # [How to Use diff --color to Change the Color of the Output](https://phoenixnap.com/kb/diff-color)
  set -x
  diff ${@:3} --color -i ./test/case/result/$2.result /tmp/miniob/result_output/$2.result.tmp
}

function main
{
  case "$1" in
    -h)
      usage
      ;;
    init)
      do_init
      ;;
    clean)
      do_clean
      ;;
    test)
      cd ./src/observer/sql/parser && bash gen_parser.sh && cd ../../../../
      echo "gen parser finish"
      do_test "$@"
      ;;
    diff)
      do_diff "$@"
      ;;
    unittest)
      do_unittest
      ;;
    make)
      cd ./src/observer/sql/parser && bash gen_parser.sh && cd ../../../../
      echo "gen parser finish"
      cd ./build && make -j8 && cd ../
      echo "make finish"
      ;;
    *)
      parse_args
      cd ./src/observer/sql/parser && bash gen_parser.sh && cd ../../../../
      echo "gen parser finish"
      build
      # clang_format # clang-format: maybe you need: sudo ln -s /usr/bin/python3 /usr/bin/python
      # check_clang_tidy
      try_make
      ;;
  esac
}

main "$@"