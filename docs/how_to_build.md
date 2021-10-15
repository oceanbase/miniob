# How to build
1. install cmake



2. build libevent

```

git submodule add https://github.com/libevent/libevent deps/libevent
cd deps
cd libevent
git checkout release-2.1.12-stable
mkdir build
cd build
cmake .. -DEVENT__DISABLE_OPENSSL=ON
make
sudo make install
```

3. build google test
```

git submodule add https://github.com/google/googletest deps/googletest
cd deps
cd googletest
mkdir build
cd build
cmake ..
make
sudo make install
```

4. build jsoncpp
```shell

git submodule add https://github.com/open-source-parsers/jsoncpp.git deps/jsoncpp
cd deps
cd jsoncpp
mkdir build
cd build
cmake -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF ..
make
sudo make install
```

5. build miniob

```shell
cd `project home`
mkdir build
cd build
cmake ..
make
```
