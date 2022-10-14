使用 ASAN 测试 MiniOB 内存问题

内存问题向来是C/C++项目非常头疼的问题，MiniOB作为C++项目，也不例外。检查内存问题的工具也非常多，Google的Address Sanitizer和Valgrind是其中两个非常优秀的工具。本文介绍Address Sanitizer（简称ASAN）的使用。

网上有非常丰富的ASAN资料，所以不对其进行过多的介绍，这里仅说明如何使用ASAN调试MiniOB。

以下使用在oceanbase/miniob docker镜像测试。

# 开启ASAN模式编译MiniOB

目前MiniOB已经支持ASAN编译，操作方法是在执行cmake命令时，增加ENABLE_ASAN选项：

```bash
cmake -DENABLE_ASAN=ON ..
```

然后执行make命令，后续操作与普通的编译相同。

ASAN 提供了丰富的选项检测各种内存问题，下面演示默认参数情况下如何检测内存问题。

# 借助ASAN工具检测错误

开启ENABLE_ASAN编译后，按照正常的测试流程运行observer和obclient来测试即可。

启动observer:

```bash
./bin/observer -s miniob.sock -f ../etc/observer.ini &
```

启动obclient:

```bash
./bin/obclient -s miniob.sock
```

在客户端执行一些测试：

```bash
miniob > create table t (id int);
SUCCESS
miniob > insert into t values(4);
SUCCESS
miniob > exit
```

接下来停掉observer:

```bash
[root@a579a8758aa1 build]# ps -ef | grep observer
root       1413      8  0 12:06 ?        00:00:00 ./bin/observer -s miniob.sock -f ../etc/observer.ini
root       1427      8  0 12:07 ?        00:00:00 grep --color=auto observer
[root@a579a8758aa1 build]# kill 1413
```

接着就可以在屏幕上看到ASAN输出的一些错误信息：

```bash
=================================================================
==1413==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 7 byte(s) in 3 object(s) allocated from:
    #0 0x7fe9e083bda0 in strdup (/lib64/libasan.so.5+0x3bda0)
    #1 0x45e66e in yylex /root/source/miniob-me/build/src/observer/lex_sql.l:74
    #2 0x46d5d2 in yyparse /root/source/miniob-me/build/src/observer/yacc_sql.tab.c:1221
    #3 0x470920 in sql_parse /root/source/miniob-me/build/src/observer/yacc_sql.y:584
    #4 0x463d60 in parse(char const*, Query*) /root/source/miniob-me/src/observer/sql/parser/parse.cpp:400
    #5 0x4668b0 in ParseStage::handle_request(common::StageEvent*) /root/source/miniob-me/src/observer/sql/parser/parse_stage.cpp:132
    #6 0x466fb8 in ParseStage::handle_event(common::StageEvent*) /root/source/miniob-me/src/observer/sql/parser/parse_stage.cpp:90
    #7 0x4716a0 in PlanCacheStage::handle_event(common::StageEvent*) /root/source/miniob-me/src/observer/sql/plan_cache/plan_cache_stage.cpp:99
    #8 0x43a173 in SessionStage::handle_request(common::StageEvent*) /root/source/miniob-me/src/observer/session/session_stage.cpp:171
    #9 0x43b2e9 in SessionStage::handle_event(common::StageEvent*) /root/source/miniob-me/src/observer/session/session_stage.cpp:105
    #10 0x7fe9e04d2a54 in common::Threadpool::run_thread(void*) /root/source/miniob-me/deps/common/seda/thread_pool.cpp:309
    #11 0x7fe9e00081ce in start_thread (/lib64/libpthread.so.0+0x81ce)

SUMMARY: AddressSanitizer: 7 byte(s) leaked in 3 allocation(s).
```

这里ASAN输出的是一个内存泄露的问题。

当然ASAN还可以检测一些其它的内存问题，比如让人头疼的内存越界。

如果在平时的训练中，出现一些异常场景，可以打开ASAN来测试看看，或许会有意外发现。



# FAQ

我在某个平台测试时，发现在编译单元测试程序的时候遇到这个问题：

ASan runtime does not come first in initial library list; you should either link runtime to your application or manually preload it with LD_PRELOAD.

网上也有一些解决方法，但是我的环境有点特别，系统会自动给我的程序增加一些默认的链接库，占据了“第一”的位置，所以asan链接库就不是第一个了，也就遇到了上面的错误。

我看了下单测cmake中 gtest_discover_tests 命令，会执行编译出来的单元测试程序，获取测试列表，但是现在的程序还不能直接运行。解决方法比较简单，可以使用下面的命令，跳过asan要求在动态链接库第一的位置：

```bash
ASAN_OPTIONS=verify_asan_link_order=0 make
```

当然，如果自己在运行某个程序时，遇到上面的问题，也可以使用这个命令，或者使用下面这个命令解决：

```bash
LD_PRELOAD=/your/path/to/libasan.so your-command
```

libasan.so 通常在跟gcc同一个基础目录下，比如gcc在/usr/bin, 那libasan.so 可能在/usr/lib64目录中。


# 参考资料

《AddressSanitizer: A Fast Address Sanity Checker》 ASAN的论文

[AddressSanitizerFlags](https://github.com/google/sanitizers/wiki/AddressSanitizerFlags) ASAN 的一些选项

