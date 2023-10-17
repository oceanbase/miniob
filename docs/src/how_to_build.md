# 如何编译

0. base

假设系统上已经安装了make等编译工具。

MiniOB 需要使用：
- cmake 版本 >= 3.13
- gcc/clang gcc建议8.3以上，编译器需要支持c++20新标准
- flex (2.5+), bison (3.7+) 用于生成词法语法分析代码

1. 环境初始化
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


2. 编译

执行下面的命令即可完成编译：
```bash
bash build.sh
```

此命令将编译一个DEBUG版本的miniob。如果希望编译其它版本的，可以参考 `bash build.sh -h`，比如：
```bash
bash build.sh release
```

此命令将编译release版本的miniob。

3. 运行
参考 [如何运行](how_to_run.md)
