---
title: 如何新增一种类型的SQL语句
---

> 本文介绍如何新增一种类型的SQL语句。

当前的SQL实现已经比较复杂，这里以新增一个简单的SQL语句为例，介绍如何新增一种类型的SQL语句。
在介绍如何新增一种类型的SQL语句之前，先介绍一下MiniOB的SQL语句的执行流程。

# SQL语句执行流程
MiniOB的SQL语句执行流程如下图所示：

> 左侧是执行流程节点，右侧是各个执行节点输出的数据结构。

```
┌──────────────────┐      ┌──────────────────┐
│       SQL        │ ---> │     String       │
└────────┬─────────┘      └──────────────────┘
         │
┌────────▼─────────┐      ┌──────────────────┐
│      Parser      │ ---> │   ParsedSqlNode  |
└────────┬─────────┘      └──────────────────┘
         │
         │
┌────────▼─────────┐      ┌──────────────────┐
│     Resolver     │ ---> │     Statement    │
└────────┬─────────┘      └──────────────────┘
         │
         │
┌────────▼─────────┐      ┌──────────────────┐
│   Transformer    │ ---> │ LogicalOperator  |
└────────┬─────────┘      │ PhysicalOperator │
         │                │       or         │
┌────────▼─────────┐      │ CommandExecutor  |
│    Optimizer     │ ---> │                  │
└────────┬─────────┘      └──────────────────┘
         │
┌────────▼─────────┐      ┌──────────────────┐
│     Executor     │ ---> │    SqlResult     │
└──────────────────┘      └──────────────────┘
```

1. 我们收到了一个SQL请求，此请求以字符串形式存储;
2. 在Parser阶段将SQL字符串，通过词法解析(lex_sql.l)与语法解析(yacc_sql.y)解析成ParsedSqlNode(parse_defs.h);
3. 在Resolver阶段，将ParsedSqlNode转换成Stmt(全称 Statement， 参考 stmt.h);
4. 在Transformer和Optimizer阶段，将Stmt转换成LogicalOperator，优化后输出PhysicalOperator(参考 optimize_stage.cpp)。如果是命令执行类型的SQL请求，会创建对应的 CommandExecutor(参考 command_executor.cpp);
5. 最终执行阶段 Executor，工作量比较少，将PhysicalOperator(物理执行计划)转换为SqlResult(执行结果)，或者将CommandExecutor执行后通过SqlResult输出结果。

# 新增一种类型的SQL语句
这里将以CALC类型的SQL为例介绍。

CALC 不是一个标准的SQL语句，它的功能是计算给定的一个四则表达式，比如：
```sql
CALC 1+2*3
```

CALC 在SQL的各个流程中，与SELECT语句非常类似，因此在增加CALC语句时，可以参考SELECT语句的实现。

首先在Parser阶段，我们需要考虑词法分析和语法分析。CALC 中需要新增的词法不多，简单的四则运算中只需要考虑数字、运算符号和小括号即可。在增加CALC之前，只有运算符号没有全部增加，那我们加上即可。参考 `lex_sql.l`。
```c
"+" |
"-" |
"*" |
"/"    { return yytext[0]; }
```

在语法分析阶段，我们参考SELECT相关的一些解析，在yacc_sql.y中增加CALC的一些解析规则，以及在parse_defs.h中SELECT解析后的数据类型，编写CALC解析出来的数据类型。
当前CALC是已经实现的，可以直接在parse_defs.h中的类型定义。
yacc_sql.y中需要增加calc_stmt，calc_stmt的类型，以及calc_stmt的解析规则。由于calc_stmt涉及到表达式运算，所以还需要增加表达式的解析规则。具体可以参考yacc_sql.y中关于calc的代码。

在语法解析结束后，输出了`CalcSqlNode`，后面resolver阶段，将它转换为Stmt，这里就是新增`CalcStmt`。通常在resolver阶段会校验SQL语法树的合法性，比如查询的表是否存在，运算类型是否正确。在`CalcStmt`中，逻辑比较简单，没有做任何校验，只是将表达式记录下来，并且认为这里的表达式都是值类型的计算。

在Transformer和Optmize阶段，对于查询类型的SQL会生成LogicalOperator和PhysicalOperator，而对于命令执行类型的SQL会生成CommandExecutor。CALC是查询类型的SQL，参考SELECT的实现，在SELECT中，有PROJECT、TABLE_SCAN等类型的算子，而CALC比较简单，我们新增`CalcLogicalOperator`和`CalcPhysicalOperator`。

由于具有执行计划的SQL，在Executor阶段，我们仅需要给SqlResult设置对应的TupleSchema即可，可以参考`ExecuteStage::handle_request_with_physical_operator`。

总结一下，新增一种类型的SQL，需要在以下几个地方做修改：
1. 词法解析，增加新的词法规则(lex_sql.l);
2. 语法解析，增加新的语法规则(yacc_sql.y);
3. 增加新的SQL语法树类型(parse_defs.h);
4. 增加新的Stmt类型(stmt.h);
5. 增加新的LogicalOperator和PhysicalOperator(logical_operator.h, physical_operator.h, optimize_stage.cpp，如果有需要);
6. 增加新的CommandExecutor(command_executor.cpp，如果有需要);
7. 设置SqlResult的TupleSchema(execute_stage.cpp，如果有需要)。
