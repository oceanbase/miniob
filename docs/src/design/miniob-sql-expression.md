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
比如 `CALC 1+2`，记录为"CALC 1+2"。

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

接下来分析 `calc_stmt` 的语法规则。
```yacc
calc_stmt:
    CALC expression_list
    {
      $$ = new ParsedSqlNode(SCF_CALC); // CALC的最终类型还是一个ParsedSqlNode
      std::reverse($2->begin(), $2->end()); // 由于左递归的原因，我们需要得出列表内容后给它反转一下
      // 直接从 expression_list 中拿出数据到目标结构中，省的再申请释放内存
      $$->calc.expressions.swap(*$2); 
      delete $2; // expression_list 本身的内存不要忘记释放掉
    }
    ;
```

`CALC expression_list` CALC 仅仅是一个关键字。expression_list 是表达式列表，我们需要对它的规则作出说明，还要在%union和 %type 中增加其类型说明。
```yacc
%union {
  ...
  std::vector<Expression *> *expression_list;
  ...
}
...
%type <expression_list> expression_list
```

expression_list 的规则如下：
```yacc
expression_list:
    expression  // 表达式列表可以是单个表达式
    {
      $$ = new std::vector<Expression*>;
      $$->emplace_back($1);
    }
    // 表达式列表也可以是逗号分隔的多个表达式，使用递归定义的方式说明规则
    | expression COMMA expression_list 
    {
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new std::vector<Expression *>;  // 表达式列表的最终类型
      }
      $$->emplace_back($1); // 目标结果中多了一个元素。
    }
    ;
```

expression 的规则会比较简单，加减乘除，以及负号取反。这里与普通的规则不同的是，我们需要关心运算符的优先级，以及负号运算符的特殊性。

优先级规则简单，乘除在先，加减在后，如果有括号先计算括号的表达式。

```yacc
%left '+' '-'
%left '*' '/'
```

%left 表示左结合，就是遇到指定的符号，先跟左边的符号结合。而定义的顺序就是优先级的顺序，越靠后的优先级越高。

负号运算符的特殊性除了它的优先级，还有它的结合性。普通的运算，比如 `1+2`，是两个数字即两个表达式，一个运算符。而负号的表示形式是 `-(1+2)`，即一个符号，一个表达式。
```yacc
%nonassoc UMINUS
```
表示 `UMINUS` 是一个一元运算符，没有结合性。在.y中，放到了 `%left '*' '/'` 的后面，说明优先级比乘除运算符高。

expression 的规则如下：
```yacc
expression '+' expression {
      $$ = create_arithmetic_expression(ArithmeticExpr::Type::ADD, $1, $3, sql_string, &@$);
    }
    | expression '-' expression {
      $$ = create_arithmetic_expression(ArithmeticExpr::Type::SUB, $1, $3, sql_string, &@$);
    }
    | expression '*' expression {
      $$ = create_arithmetic_expression(ArithmeticExpr::Type::MUL, $1, $3, sql_string, &@$);
    }
    | expression '/' expression {
      $$ = create_arithmetic_expression(ArithmeticExpr::Type::DIV, $1, $3, sql_string, &@$);
    }
    | LBRACE expression RBRACE {  // '(' expression ')'
      $$ = $2;
      $$->set_name(token_name(sql_string, &@$));
    }
    // %prec 告诉yacc '-' 负号预算的优先级，等于UMINUS的优先级
    | '-' expression %prec UMINUS {
      $$ = create_arithmetic_expression(ArithmeticExpr::Type::NEGATIVE, $2, nullptr, sql_string, &@$);
    }
    | value {
      $$ = new ValueExpr(*$1);
      $$->set_name(token_name(sql_string, &@$));
      delete $1;
    }
    ;
```

`create_arithmetic_expression` 是一个创建算术表达式的函数，它的实现在.y文件中，不再罗列。

表达式的名称，需要在输出结果中展示出来。我们知道当前的SQL语句，也知道某个token的开始列号与截止列号，就可以计算出来这个表达式对应的SQL命令输入是什么。在 expression 规则描述中，就是 `$$->set_name(token_name(sql_string, &@$));`，其中 `sql_string` 就是当前的SQL语句，@$ 是当前的token的位置信息。

## 抽象表达式类型
上面语法分析中描述的都是算术表达式，但是真实的SQL语句中，像字段名、常量、比较运算、函数、子查询等都是表达式。我们需要定义一个基类，然后派生出各种表达式类型。

这些表达式的定义已经在expression.h中定义，但是没有在语法解析中体现。更完善的做法是在 select 的属性列表、where 条件、insert 的 values等语句中，都使用表达式来表示。

## '-' 缺陷
由于在词法分析中，负号'-'与数字放在一起时，会被认为是一个负值数字，作为一个完整的token返回给语法分析，所以当前的语法分析无法正确的解析下面的表达式：
```
1 -2;
```
这个表达式的结果应该是 -1，但是当前的语法分析会认为是两个表达式，一个是1，一个是-2，这样就无法正确的计算出结果。
当前修复此问题的成本较高，需要修改词法分析的规则，所以暂时不做处理。