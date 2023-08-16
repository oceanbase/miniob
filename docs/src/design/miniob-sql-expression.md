> 本文介绍如何解析表达式

# 介绍
表达式是SQL操作中非常基础的内容。
我们常见的表达式就是四则运算的表达式，比如`1+2`、`3*(10-3)`等。在常见的数据库中，比如MySQL、[OceanBase](https://github.com/oceanbase/oceanbase)，可以运行 `select 1+2`、`select 3*(10-3)`，来获取这种表达式的结果。但是同时在SQL中，也可以执行 `select 1`、`select field1 from table1`，来查询一个常量或者一个字段。那我们就可以把表达式的概念抽象出来，认为常量、四则运算、表字段、函数调用等都是表达式。

# MiniOB 中的表达式实现
当前MiniOB并没有实现上述的所有类型的表达式，而是选择扩展SQL语法，增加了 `CALC` 命令，以支持算术表达式运算。这里就以 CALC 支持的表达式为例，介绍如何在 MiniOB 中实现表达式。

> 本文的内容会有一部分与 [如何新增一种类型的SQL语句](./miniob-how-to-add-new-sql.md) 重复，但是这里会更加详细的介绍表达式的实现。

这里假设大家对 MiniOB 的SQL运行过程有一定的了解，如果没有，可以参考 [如何新增一种类型的SQL语句](./miniob-how-to-add-new-sql.md) 的第一个部分。

在介绍实现细节之前先看下一个例子以及它的执行结果：
```sql
CALC 1+2
```
执行结果：
```
1+2
3
```

注意这个表达式输出时会输出表达式的原始内容。

## SQL 语句
MiniOB 从客户端接收到SQL请求后，会创建 `SessionEvent`，其中 `query_` 以字符串的形式保存了SQL请求。
比如 `CALC 1+2`，"CALC 1+2"。

## SQL Parser
Parser部分分为词法分析和语法分析。

如果对词法分析语法分析还不了解，建议先查看 [SQL Parser](./miniob-sql-parser.md)。

### 词法分析
算术表达式需要整数、浮点数，以及加减乘除运算符。我们在lex_sql.l中可以看到 NUMBER 和 FLOAT的token解析。运算符的相关模式匹配定义如下：

```lex
"+" |
"-" |
"*" |
"/"    { return yytext[0]; }
```

### 语法分析
因为 CALC 也是一个完整的SQL语句，那我们先给它定义一个类型。我们定义一个 CALC 语句可以计算多个表达式的值，表达式之间使用逗号分隔，那 CalcSqlNode 定义应该是这样的：
```cpp
struct CalcSqlNode
{
  std::vector<Expression *> expressions;
};
```

在 yacc_sql.y 文件中，我们增加一种新的语句类型 `calc_stmt`，与SELECT类似。它的类型也是`sql_node`：
```yacc
%type <sql_node> calc_stmt
```


## Resolver

## Transformer & Optimizer

## Executor
