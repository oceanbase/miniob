# 如何编译

0. base

假设系统上已经安装了make等编译工具。

MiniOB 需要使用：
- cmake 版本 >= 3.10
- gcc/clang gcc建议8.3以上，编译器需要支持c++20新标准

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

**FAQ**
1. 运行observer出现找不到链接库
A: 由于安装依赖时，默认安装在 `/usr/local/` 目录下，而环境变量中没有将这个目录包含到动态链接库查找路径。可以将下面的命令添加到 HOME 目录的 `.bashrc` 中：
```bash
export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
```
然后执行 `source ~/.bashrc` 加载环境变量后重新启动程序。

LD_LIBRARY_PATH 是Linux环境中，运行时查找动态链接库的路径，路径之间以冒号':'分隔。

将数据写入 bashrc 或其它文件，可以在下次启动程序时，会自动加载，而不需要再次执行 source 命令加载。

> NOTE: 如果你的终端脚本使用的不是bash，而是zsh，那么就需要修改 .zshrc。
