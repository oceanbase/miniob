---
title: 调试
author: caizj
---

# miniob调试篇

调试c/c++程序，常用的有两种方式，一是打印日志调试，二是gdb调试，调试不仅可以定位问题，也可以用来熟悉代码。

## miniob关键代码

首先，拿到一份陌生的代码，要先确定代码的大致结构，一些关键数据结构和方法，这里的技巧和经验不展开了

### miniob的关键数据结构

部分关键数据结构：

```c++
parse_def.h:
	struct Selects;//查询相关
	struct CreateTable;//建表相关
	struct DropTable;//删表相关
	enum SqlCommandFlag;//sql语句对应的command枚举
	union Queries;//各类dml和ddl操作的联合
table.h
	class Table;
db.h
	class Db;
```

### miniob的关键接口

部分关键接口

```c++
RC parse(const char *st, Query *sqln);//sql parse入口
ExecuteStage::handle_request
ExecuteStage::do_select
DefaultStorageStage::handle_event
DefaultHandler::create_index
DefaultHandler::insert_record
DefaultHandler::delete_record
DefaultHandler::update_record
Db::create_table
Db::find_table
Table::create
Table::scan_record
Table::insert_record
Table::update_record
Table::delete_record
Table::scan_record
Table::create_index
```

## 打印日志调试

### miniob提供的日志接口

```c++
deps/common/log/log.h:
#define LOG_PANIC(fmt, ...)
#define LOG_ERROR(fmt, ...)
#define LOG_WARN(fmt, ...)
#define LOG_INFO(fmt, ...)
#define LOG_DEBUG(fmt, ...)
#define LOG_TRACE(fmt, ...)
```

日志相关配置项`observer.ini`

```
LOG_FILE_NAME = observer.log
#  LOG_LEVEL_PANIC = 0,
#  LOG_LEVEL_ERR = 1,
#  LOG_LEVEL_WARN = 2,
#  LOG_LEVEL_INFO = 3,
#  LOG_LEVEL_DEBUG = 4,
#  LOG_LEVEL_TRACE = 5,
#  LOG_LEVEL_LAST
LOG_FILE_LEVEL=5
LOG_CONSOLE_LEVEL=1
```

### 在日志中输出调用栈

`lbt` 函数可以获取当前调用栈，可以在日志中输出调用栈，方便定位问题。

比如
```c++
LOG_DEBUG("debug lock %p, lbt=%s", &lock_, lbt());
```

可能得到下面的日志

```
unlock@mutex.cpp:273] >> debug unlock 0xffffa40fe8c0, lbt=0x4589c 0x5517f8 0x5329e0 0x166308 0x162c2c 0x210438 0x204ee0 0x2373b0 0x2373d0 0x203efc 0x203d88 0x223f6c 0x242fc8 0x274810 0x32bd58 0xca028 0x2dbcf8 0x2da614 0xbbf30 0x2cb908 0x2d4408 0x2d43dc 0x2d435c 0x2d431c 0x2d42d4 0x2d4270 0x2d4244 0x2d4224 0xd31fc 0x7d5c8 0xe5edc
```

可以用`addr2line`工具来解析地址，比如

```bash
addr2line -pCfe ./bin/observer 0x4589c 0x5517f8 0x5329e0 0x166308 0x162c2c 0x210438 0x204ee0 0x2373b0 0x2373d0 0x203efc 0x203d88 0x223f6c 0x242fc8 0x274810 0x32bd58 0xca028 0x2dbcf8 0x2da614 0xbbf30 0x2cb908 0x2d4408 0x2d43dc 0x2d435c 0x2d431c 0x2d42d4 0x2d4270 0x2d4244 0x2d4224 0xd31fc 0x7d5c8 0xe5edc
?? ??:0
common::lbt() at /root/miniob/deps/common/log/backtrace.cpp:118
common::DebugMutex::unlock() at /root/miniob/deps/common/lang/mutex.cpp:273 (discriminator 25)
Frame::write_unlatch(long) at /root/miniob/src/observer/storage/buffer/frame.cpp:113
Frame::write_unlatch() at /root/miniob/src/observer/storage/buffer/frame.cpp:88
RecordPageHandler::cleanup() at /root/miniob/src/observer/storage/record/record_manager.cpp:262
RecordPageHandler::~RecordPageHandler() at /root/miniob/src/observer/storage/record/record_manager.cpp:96
RowRecordPageHandler::~RowRecordPageHandler() at /root/miniob/src/observer/storage/record/record_manager.h:280
RowRecordPageHandler::~RowRecordPageHandler() at /root/miniob/src/observer/storage/record/record_manager.h:280
...
```

> 注意：./bin/observer 是你的可执行文件路径，这里是一个示例，实际路径可能不同。

## gdb调试

调试工具有很多种，但是它们的关键点都是类似的，比如关联到进程、运行时查看变量值、单步运行、跟踪变量等。GDB是在Linux环境中常用的调试工具。其它环境上也有类似的工具，比如LLDB，或者Windows可能使用Visual Studio直接启动调试。Java的调试工具是jdb。

另外，很多同学喜欢使用Visual Studio Code(vscode)开发项目，vscode提供了很多插件，包括调试的插件，这些调试插件支持gdb、lldb等，可以按照自己的平台环境，设置不同的调试工具。

这里介绍了gdb的基本使用，其它工具的使用方法类似。

1. Attach进程

    ```
    [caizj@localhost run]$ gdb -p `pidof observer` 
    
    GNU gdb (GDB) Red Hat Enterprise Linux 8.2-15.el8 Copyright (C) 2018 Free Software Foundation, Inc.
    
    (gdb)
    ```

2. 设置断点

    ```
    (gdb) break do_select
    Breakpoint 1 at 0x44b636: file /home/caizj/source/stunning-engine/src/observer/sql/executor/execute_stage.cpp, line 526.
    (gdb) info b
    Num     Type           Disp Enb Address            What
    1       breakpoint     keep y   0x000000000044b636 in ExecuteStage::do_select(char const*, Query*, SessionEvent*)
                                                       at /home/caizj/source/stunning-engine/src/observer/sql/executor/execute_stage.cpp:526
    ```

    ```
    (gdb) break Table::scan_record
    Breakpoint 2 at 0x50b82b: Table::scan_record. (2 locations)
    (gdb) inf b
    Num     Type           Disp Enb Address            What
    1       breakpoint     keep y   0x000000000044b636 in ExecuteStage::do_select(char const*, Query*, SessionEvent*)
                                                       at /home/caizj/source/stunning-engine/src/observer/sql/executor/execute_stage.cpp:526
    2       breakpoint     keep y   <MULTIPLE>
    2.1                         y     0x000000000050b82b in Table::scan_record(Trx*, ConditionFilter*, int, void*, void (*)(char const*, void*))
                                                       at /home/caizj/source/stunning-engine/src/observer/storage/common/table.cpp:421
    2.2                         y     0x000000000050ba00 in Table::scan_record(Trx*, ConditionFilter*, int, void*, RC (*)(Record*, void*))
                                                       at /home/caizj/source/stunning-engine/src/observer/storage/common/table.cpp:426
    (gdb)
    ```

3. 继续执行

    ```
    (gdb) c
    Continuing.
    ```

4. 触发断点

    执行：miniob > select * from t1;

    ```
    [Switching to Thread 0x7f51345f9700 (LWP 54706)]
    
    Thread 8 "observer" hit Breakpoint 1, ExecuteStage::do_select (this=0x611000000540,
        db=0x6040000005e0 "sys", sql=0x620000023080, session_event=0x608000003d20)
        at /home/caizj/source/stunning-engine/src/observer/sql/executor/execute_stage.cpp:526
    526	  RC rc = RC::SUCCESS;
    (gdb)
    ```

5. 单步调式

    ```
    575	  std::vector<TupleSet> tuple_sets;
    (gdb) next
    576	  for (SelectExeNode *&node: select_nodes) {
    (gdb) n
    577	    TupleSet tuple_set;
    (gdb)
    578	    rc = node->execute(tuple_set);
    (gdb)
    ```

6. 跳入
  跟踪到函数内部

    ```
    (gdb) s
    SelectExeNode::execute (this=0x60700002ce80, tuple_set=...)
        at /home/caizj/source/stunning-engine/src/observer/sql/executor/execution_node.cpp:43
    43	  CompositeConditionFilter condition_filter;
    (gdb)
    
    ```

7. 打印变量

    ```
    (gdb) p tuple_set
    $3 = (TupleSet &) @0x7f51345f1760: {tuples_ = std::vector of length 0, capacity 0, schema_ = {
        fields_ = std::vector of length 0, capacity 0}}
    (gdb)
    ```

8. watch变量

    ```
    (gdb) n
    443	  RC rc = RC::SUCCESS;
    (gdb) n
    444	  RecordFileScanner scanner;
    (gdb) n
    445	  rc = scanner.open_scan(*data_buffer_pool_, file_id_, filter);
    (gdb) watch -l rc
    Hardware watchpoint 3: -location rc
    (gdb) c
    Continuing.
    
    Thread 8 "observer" hit Hardware watchpoint 3: -location rc
    
    Old value = SUCCESS
    New value = RECORD_EOF
    0x000000000050c2de in Table::scan_record (this=0x60f000007840, trx=0x606000009920,
        filter=0x7f51345f12a0, limit=2147483647, context=0x7f51345f11c0,
        record_reader=0x50b74a <scan_record_reader_adapter(Record*, void*)>)
        at /home/caizj/source/stunning-engine/src/observer/storage/common/table.cpp:454
    454	  for ( ; RC::SUCCESS == rc && record_count < limit; rc = scanner.get_next_record(&record)) {
    (gdb)
    ```

9. 结束函数调用

    ```
    (gdb) finish
    Run till exit from #0  0x000000000050c2de in Table::scan_record (this=0x60f000007840,
        trx=0x606000009920, filter=0x7f51345f12a0, limit=2147483647, context=0x7f51345f11c0,
        record_reader=0x50b74a <scan_record_reader_adapter(Record*, void*)>)
        at /home/caizj/source/stunning-engine/src/observer/storage/common/table.cpp:454
    ```

10. 结束调试

    ```
    (gdb) quit
    A debugging session is active.
    
    	Inferior 1 [process 54699] will be detached.
    
    Quit anyway? (y or n) y
    Detaching from program: /home/caizj/local/bin/observer, process 54699
    [Inferior 1 (process 54699) detached]
    ```

### Visual Studio Code 调试
代码中已经为vscode配置了launch.json，可以直接启动调试。
launch.json中有两个调试配置，一个是Debug，一个是LLDB。其中Debug使用cppdbg，会自动探测调试工具gdb或lldb，而LLDB会使用lldb调试工具。通常情况下，大家使用Debug就可以了，但是我在测试过程中发现cppdbg不能在macos上正常工作，因而增加了LLDB的配置，以便在macos上调试，如果使用macos的同学，可以使用LLDB配置启动调试程序。

#### MiniOB 中的 tasks

vscode 可以非常方便的运行任务(task)来运行预配置的命令，比如shell。
miniob 的编译也可以通过脚本来执行(build.sh)。这里预配置了几个编译任务，可以按需自取，也可以按照需要，增加新的配置，运行自己的参数。

下面是一个 debug 模式编译的示例，也是vscode工程默认的Build配置。这里做个简单介绍，以方便大家有需要的时候，修改配置满足自己需要。
其中 
- `label` 是一个任务名字，在 `Run task`的时候，可以看到
- `type` 表示任务的类型。这里是一个shell脚本
- `command` 这里是一个shell脚本的话，那command就是运行的命令，跟我们在终端上执行是一样的效果
- `problemMatcher` 告诉vscode如何定位问题。这里不用设置，vscode可以自动检测
- `group` 使用vscode将此任务设置为默认Build任务时，vscode自己设置上来的，不需要调整。

```json
{
    "label": "build_debug",
    "type": "shell",
    "command": "bash build.sh debug",
    "problemMatcher": [],
    "group": {
        "kind": "build",
        "isDefault": true
    }
}
```

#### MiniOB 中的 launch

很多同学不习惯使用gdb的终端界面来调试程序，那么在 vscode 中调试miniob非常方便，与Visual Studio、Clion中类似，都有一个操作界面。
vscode中启动调试程序是通过launch.json来配置的，这里简单介绍一下主要内容。

下面是截取的一段关键内容。这里介绍一些关键字段
- `type` 当前调试使用哪种类型。这里是lldb (我个人习惯了gdb，但是我没有找到，也不想找了)
- `name` 这里会显示在vscode调试窗口启动时的名字中
- `program` 要调试的程序。对miniob来说，我们通常都是调试服务端代码，这里就是observer的路径。workspaceFolder 是当前工程的路径，defaultBuildTask 是默认构建的任务名称，与我们的构建路径刚好一致。observer 是编译完成安装在构建路径的bin下。
- `args` 启动程序时的命令行参数。在终端上，大家也可以这么启动observer: `./bin/observer -f ../etc/observer.ini -s miniob.sock
- `cwd` observer 运行时的工作目录，就是在observer程序中获取当前路径时，就会是这个路径。

```json
{
    "type": "lldb",
    "request": "launch",
    "name": "Debug",
    "program": "${workspaceFolder}/${defaultBuildTask}/bin/observer",
    "args": ["-f", "${workspaceFolder}/etc/observer.ini", "-s", "miniob.sock"],
    "cwd": "${workspaceFolder}/${defaultBuildTask}/"
}
```

注意，如果要调试 release 或者其它任务编译出来的observer，就需要调整这个文件，或者新增一个配置，因为这个配置文件指定的observer路径是默认的build。
