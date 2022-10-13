# How to build

0. base

假设系统上已经安装了make等编译工具。

MiniOB 需要使用：
- cmake 版本 >= 3.10
- gcc/clang gcc建议8.3以上，编译器需要支持c++14等新标准

获取MiniOB的submodule
```bash
cd `project home`
git submodule init
git submodule update
# or
git submodule update --init

# 或者通过以下方式自动初始化并更新仓库中的每一个子模块
git clone --recurse-submodules https://github.com/oceanbase/miniob.git
```

1. install cmake

需要安装了3.10或以上版本的cmake，可以跳过此步骤。
在[cmake官网](https://cmake.org/download/) 下载对应系统的cmake然后安装。
比如
```bash
wget https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0-linux-x86_64.sh
bash cmake-3.24.0-linux-x86_64.sh
```

如果是mac系统，可以执行下面的命令安装：

```bash
brew install cmake
```

2. install gcc

如果是个人学习使用，clang通常没有问题，但是如果是参加比赛或者OceanBase官网训练营使用，建议安装GCC。因为clang也某些情况下的表现会与GCC不一致。
另外，如果GCC的版本比较旧，也需要安装较新版本的GCC。有些比较旧的操作系统，比如CentOS 7，自带的编译器版本是4.8.5，对C++14等新标准，支持不太好，建议升级GCC。
在Linux系统上，通常使用 `yum install gcc gcc-g++` 就可以安装gcc，但是有些时候安装的编译器版本比较旧，还需要手动。

- 如何查看GCC版本
```bash
gcc --version
```

- 下载GCC源码


可以在[GCC 官网](https://gcc.gnu.org/)浏览，找到镜像入口，选择速度比较快的镜像，比如 [Russia, Novosibirsk GCC镜像链接](http://mirror.linux-ia64.org/gnu/gcc/releases/)，然后选择自己的版本。
建议选择8.3或以上版本。
因为miniob是为了学习使用，所以可以选择比较新的GCC，不追求稳定。

- 编译GCC源码

> NOTE: 编译高版本的GCC时，需要本地也有一个稍高一点版本的GCC。比如编译GCC 11，本地的GCC需要能够支持stdc++11才能编译。可以先编译GCC 5.4，然后再编译高版本。

如果你看到这里想放弃，可以选择MiniOB提供的[docker镜像](https://hub.docker.com/r/oceanbase/miniob)，其中自带GCC 11。

下载的代码文件，比如是 gcc-11.3.0.tar.gz

```bash
# 解压
tar xzvf gcc-11.3.0.tar.gz
cd gcc-11.3.0

# 下载依赖
./contrib/download_prerequisites

# 配置。可以通过prefix参数设置编译完成的GCC的安装目录，如果不指定，会安装在/usr/local下
# 可以配置为当前用户的某个目录
./configure --prefix=/your/new/gcc/path --enable-threads=posix --disable-checking \
    --disable-multilib --enable--long-long --with-system-zlib --enable-languages=c,c++

# 开始编译
make -j

# 安装
# 编译产生物会安装到 configure --prefix指定的目录中，或系统默认目录下
make install

# 修改环境变量
# 可以将下面的配置写到 .bashrc 或 .bash_profile 中，这样每次登录都会自动生效
export PATH=/your/new/gcc/path/bin:$PATH
export LD_LIBRARY_PATH=/your/new/gcc/path/lib64:$LD_LIBRARY_PATH
export CC=/your/new/gcc/path/bin/gcc
export CXX=/your/new/gcc/path/bin/g++

# NOTE: 上面的环境变量CC和CXX是告诉CMake能够找到我们的编译器。cmake优先查找的
#       编译器是cc而不是gcc，而一般系统中会默认带cc，所以使用环境变量告诉cmake。
#       也可以使用 cmake 参数 
#       `-DCMAKE_C_COMPILER=/your/new/gcc/path/bin/gcc -DCMAKE_CXX_COMPILER=/your/new/gcc/path/bin/g++`
#       来指定编译器
```

如果./contrib/download_prerequisites 下载时特别慢或下载失败，可以手动从官网上下载依赖，然后解压相应的包到gcc的目录下。
```
ftp://ftp.gnu.org/gnu/gmp
https://mpfr.loria.fr/mpfr-current/#download
https://www.multiprecision.org/mpc/download.html
```

3. build libevent

```bash
cd deps
cd libevent
git checkout release-2.1.12-stable
mkdir build
cd build
cmake .. -DEVENT__DISABLE_OPENSSL=ON
make
sudo make install
```

4. build google test

```bash
cd deps
cd googletest
mkdir build
cd build
cmake ..
make
sudo make install
```

5. build jsoncpp

```bash
cd deps
cd jsoncpp
mkdir build
cd build
cmake -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF ..
make
sudo make install
```

6. build miniob

```bash
cd `project home`
mkdir build
cd build
# 建议开启DEBUG模式编译，更方便调试
cmake .. -DDEBUG=ON
make
```
