#!/bin/bash

TOPDIR=`readlink -f \`dirname $0\``
BUILD_SH=$TOPDIR/build.sh

DEP_DIR=${TOPDIR}/deps
CMAKE_COMMAND="cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1"

ALL_ARGS=("$@")
BUILD_ARGS=()
MAKE_ARGS=(-j $CPU_CORES)
MAKE=make
ASAN_OPTION=ON

echo "$0 ${ALL_ARGS[@]}"

function usage
{
  echo "Usage:"
  echo "./build.sh -h"
  echo "./build.sh init"
  echo "./build.sh clean"
  echo "./build.sh [BuildType] [--make [MakeOptions]]"
  echo ""
  echo "OPTIONS:"
  echo "BuildType => debug(default), release, debug_asan, release_asan"
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
    $MAKE "${MAKE_ARGS[@]}"
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

  # build libevent
  cd ${TOPDIR}/deps/libevent && \
    git checkout release-2.1.12-stable && \
    mkdir build && \
    cd build && \
    cmake .. -DEVENT__DISABLE_OPENSSL=ON && \
    make -j4 && \
    make install

  # build googletest
  cd ${TOPDIR}/deps/googletest && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j4 && \
    make install

  # build google benchmark
  cd ${TOPDIR}/deps/benchmark && \
    mkdir build && \
    cd build && \
    cmake .. -DBENCHMARK_ENABLE_TESTING=OFF  -DBENCHMARK_INSTALL_DOCS=OFF -DBENCHMARK_ENABLE_GTEST_TESTS=OFF -DBENCHMARK_USE_BUNDLED_GTEST=OFF -DBENCHMARK_ENABLE_ASSEMBLY_TESTS=OFF && \
    make -j4 && \
    make install

  # build jsoncpp
  cd ${TOPDIR}/deps/jsoncpp && \
    mkdir build && \
    cd build && \
    cmake -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF .. && \
    make && \
    make install

  cd $current_dir
}

function prepare_build_dir
{
  TYPE=$1
  mkdir -p ${TOPDIR}/build_${TYPE} && cd ${TOPDIR}/build_${TYPE}
}

function do_build
{
  TYPE=$1; shift
  prepare_build_dir $TYPE || return
  echo "${CMAKE_COMMAND} ${TOPDIR} $@"
  ${CMAKE_COMMAND} ${TOPDIR} $@
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
    xrelease_asan)
      do_build "$@" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDEBUG=OFF -DENABLE_ASAN=$ASAN_OPTION
      ;;
    xdebug)
      do_build "$@" -DCMAKE_BUILD_TYPE=Debug -DDEBUG=ON
      ;;
    xdebug_asan)
      do_build "$@" -DCMAKE_BUILD_TYPE=Debug -DDEBUG=ON -DENABLE_ASAN=$ASAN_OPTION
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
