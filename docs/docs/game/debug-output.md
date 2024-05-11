---
title: 输出调试信息
---

本篇文档介绍如何向[训练营](https://open.oceanbase.com/train)输出调试信息。

在使用训练营提交测试时，有时候会遇到本地环境没有问题，但是训练营上的输出总是不符合预期。但是训练营没有办法调试，这时候就需要在训练营上输出调试信息，以便于定位问题。

**如何输出调试信息**

可以参考文件 `sql_debug.h`，在需要输出调试信息的地方，调用`sql_debug`函数即可。sql_debug 的使用与打印日志类似，不过可以在向客户端输出正常结果后，再输出调试信息。
执行`sql_debug`同时会在日志中打印DEBUG级别的日志。

**示例**

1. `CreateTableStmt::create`
2. `TableScanPhysicalOperator::next`

**注意**
由于训练营上能够容纳的信息有限，所以输出太多信息会被截断。

**开关**

每个连接都可以开启或关闭调试，可以参考 `Session::sql_debug_`。

在交互式命令行中，可以使用 `set sql_debug=1` 开启调试，使用 `set sql_debug=0` 关闭调试。

> 当前没有实现查看变量的命令。

示例

```bash
miniob > select * from t;
id
1

# get a tuple: 1
miniob > set sql_debug=0;
SUCCESS
miniob > select * from t;
id
1

```