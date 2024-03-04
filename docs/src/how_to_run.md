# 如何运行

编译完成后，可以在build目录（可能是build_debug或build_release）下找到bin/observer，就是我们的服务端程序，bin/obclient是自带的客户端程序。
当前服务端程序启动已经支持了多种模式，可以以TCP、unix socket方式启动，这时需要启动客户端以发起命令。observer还支持直接执行命令的模式，这时不需要启动客户端，直接在命令行输入命令即可。

**以直接执行命令的方式启动服务端程序**
```bash
./bin/observer -f ../etc/observer.ini -P cli
```
这会以直接执行命令的方式启动服务端程序，可以直接输入命令，不需要客户端。所有的请求都会以单线程的方式运行，配置项中的线程数不再有实际意义。

**以监听TCP端口的方式启动服务端程序**

```bash
./bin/observer -f ../etc/observer.ini -p 6789
```
这会以监听6789端口的方式启动服务端程序。
启动客户端程序：
```bash
./bin/obclient -p 6789
```
这会连接到服务端的6789端口。

**以监听unix socket的方式启动服务端程序**
```bash
./bin/observer -f ../etc/observer.ini -s miniob.sock
```
这会以监听unix socket的方式启动服务端程序。
启动客户端程序：
```bash
./bin/obclient -s miniob.sock
```
这会连接到服务端的miniob.sock文件。

**并发模式**

默认情况下，编译出的程序是不支持并发的。如果需要支持并发，需要在编译时增加选项 `-DCONCURRENCY=ON`:
```bash
cmake -DCONCURRENCY=ON ..
```

或者

```bash
bash build.sh -DCONCURRENCY=ON
```

然后使用上面的命令启动服务端程序，就可以支持并发了。

**启动参数介绍**

| 参数      | 说明 |
| ----------- | ----------- |
| -h | 帮助说明       |
| -f | 配置文件路径。如果不指定，就会使用默认值 ../etc/observer.ini。 |
| -p | 服务端监听的端口号。如果不指定，并且没有使用unix socket或cli的方式启动，就会使用配置文件中的值，或者使用默认值。        |
| -s | 服务端监听的unix socket文件。如果不指定，并且没有使用TCP或cli的方式启动，就会使用TCP的方式启动服务端。 |
| -P | 使用的通讯协议。当前支持文本协议(plain，也是默认值)，MySQL协议(mysql)，直接交互(cli)。<br/>使用plain协议时，请使用自带的obclient连接服务端。<br/>使用mysql协议时，使用mariadb或mysql客户端连接。<br/>直接交互模式(cli)不需要使用客户端连接，因此无法开启多个连接。  |
| -t | 事务模型。没有事务(vacuous，默认值)和MVCC(mvcc)。 使用mvcc时一定要编译支持并发模式的代码。  |
| -T | 线程模型。一个连接一个线程(one-thread-per-connection，默认值)和一个线程池处理所有连接(java-thread-pool)。 |
| -n | buffer pool 的内存大小，单位字节。 |

**更多**

observer还提供了一些其它参数，可以通过`./bin/observer -h`查看。

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