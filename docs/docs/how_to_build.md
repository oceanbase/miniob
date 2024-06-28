---
title: 如何编译
---

# 如何编译

## 0. base

假设系统上已经安装了make等编译工具。

MiniOB 需要使用：

- cmake 版本 >= 3.13
- gcc/clang gcc 11 以上，clang 14以上，编译器需要支持c++20新标准
- flex (2.5+), bison (3.7+) 用于生成词法语法分析代码

## 1. 环境初始化

如果是第一次在这个环境上编译miniob，需要安装一些miniob的依赖库，执行下面的命令即可安装：

```bash
bash build.sh init
```

脚本将自动拉取依赖库(可以参考 .gitmodules) 然后编译安装到系统目录。

如果执行用户不是root，需要在命令前加上 `sudo`：

```bash
sudo bash build.sh init
```

> 如果使用 GitPod 开发，可以跳过这步，会自动执行。

## 2. 编译

执行下面的命令即可完成编译：

```bash
bash build.sh
```

此命令将编译一个DEBUG版本的miniob。如果希望编译其它版本的，可以参考 `bash build.sh -h`，比如：

```bash
bash build.sh release
```

此命令将编译release版本的miniob。

## 3. 运行

参考 [如何运行](how_to_run.md)

## FAQ

### 1. sudo找不到cmake

**Q:**

在“1. 环境初始化”中执行命令:

```bash
sudo bash build.sh init
```

时，报错:

```bash
build.sh: line xx: cmake: command not found
```

**A:**

- 1. 检查“0. base”中cmake版本要求是否满足。

```bash
cmake --version
```

- 2. 检查是否出现了“Linux系统下执行sudo命令环境变量失效现象”。

***检查***

在当前用户和root用户下均能找到cmake，而在当前用户下sudo cmake却找不到cmake，即:

```bash
[mu@vm-cnt8:~]$ sudo -E cmake --version
[sudo] password for mu: 
sudo: cmake: command not found
```

则可能就出现了“Linux系统下执行sudo命令环境变量失效现象”，本例中具体是PATH环境变量实效（被重置），导致找不到cmake。

***解决方法：建立软链接***

- 找到执行sudo命令时的PATH变量中有哪些路径：

```bash
[mu@vm-cnt8:~]$ sudo env | grep PATH
PATH=/sbin:/bin:/usr/sbin:/usr/bin
```

- 找到cmake所在的路径：

```bash
[mu@vm-cnt8:~]$ whereis cmake
cmake: /usr/local/bin/cmake /usr/share/cmake
```

- 在PATH变量中的一个合适路径下建立指向cmake的软链接：

```bash
[root@vm-cnt8:~]# ls /usr/bin | grep cmake
[root@vm-cnt8:~]# ln -s /usr/local/bin/cmake /usr/bin/cmake
[root@vm-cnt8:~]# ll /usr/bin | grep cmake
lrwxrwxrwx. 1 root root          20 Sep  1 05:57 cmake -> /usr/local/bin/cmake
```

***验证***

```bash
$ sudo -E cmake --version
cmake version 3.27.4
```

发现sudo时能找到cmake了，此时再执行

```bash
sudo bash build.sh init
```

则不会因为找不到cmake而报错。

**更多信息：**

关于该问题的更多细节，请参考[问题来源](https://ask.oceanbase.com/t/topic/35604437/7)。
关于该问题的进一步分析，请参考[Linux系统下执行sudo命令环境变量失效现象](https://zhuanlan.zhihu.com/p/669332689)。
也可以将cmake所在路径添加到sudo的PATH变量中来解决上述问题，请参考[sudo命令下环境变量实效的解决方法](https://www.cnblogs.com/xiao-xiaoyang/p/17444600.html)。
