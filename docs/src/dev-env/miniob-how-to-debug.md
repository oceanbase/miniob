# miniob调试篇

-- by caizj

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

### 打印日志调试

miniob提供的日志接口

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

### gdb调试

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