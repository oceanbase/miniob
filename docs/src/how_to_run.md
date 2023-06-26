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

**更多**

observer还提供了一些其它参数，可以通过`./bin/observer -h`查看。
