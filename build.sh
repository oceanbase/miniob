#!/bin/bash

# readlink -f cannot work on mac
TOPDIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

BUILD_SH=$TOPDIR/build.sh

CMAKE_COMMAND="cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 --log-level=STATUS"

ALL_ARGS=("$@")
BUILD_ARGS=()
MAKE_ARGS=(-j $CPU_CORES)
MAKE=make

echo "$0 ${ALL_ARGS[@]}"

function usage
{
  echo "Usage:"
  echo "./build.sh -h"
  echo "./build.sh init # install dependence"
  echo "./build.sh clean"
  echo "./build.sh [BuildType] [--make [MakeOptions]]"
  echo ""
  echo "OPTIONS:"
  echo "BuildType => debug(default), release"
  echo "MakeOptions => Options to make command, default: -j N"

  echo ""
  echo "Examples:"
  echo "# Init."
  echo "./build.sh init"
  echo ""
  echo "# Build by debug mode and make with -j24."
  echo "./build.sh debug --make -j24"
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
    *)
      parse_args
      build
      try_make
      ;;
  esac
}

main "$@"
