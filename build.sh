#!/bin/bash
set -e
PROJECT_HOME=`pwd`
CPU_COUNT=`grep -c ^processor /proc/cpuinfo`

# TODO : To install dependence and support more os
# TODO : If observer is running
# kill -9 `pidof observer` 

# update submodules
git submodule update --init

# build libevent
cd deps/libevent
rm -rf build
git checkout release-2.1.12-stable
mkdir build
cd build
cmake .. -DEVENT__DISABLE_OPENSSL=ON
make -j $CPU_COUNT
sudo make install

# build google test
cd $PROJECT_HOME
cd deps/googletest
rm -rf build
mkdir build
cd build
cmake ..
make -j $CPU_COUNT
sudo make install

# build jsoncpp
cd $PROJECT_HOME
cd deps/jsoncpp
rm -rf build
mkdir build
cd build
cmake -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF ..
make -j $CPU_COUNT
sudo make install

# build miniob
cd $PROJECT_HOME
rm -rf build
mkdir build
cd build
# 建议开启DEBUG模式编译，更方便调试
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j $CPU_COUNT

# run
cd $PROJECT_HOME
nohup ./build/bin/observer -f ./etc/observer.ini 2>&1 &
sleep 3

./build/bin/obclient
