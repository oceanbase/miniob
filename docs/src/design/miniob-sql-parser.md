> 这部分内容会介绍一些如何对miniob中的词法语法分析模块进行开发与调试，以及依赖的工具。

# 简介
SQL 解析分为词法分析与语法分析，与编译原理中介绍的类似。这里主要介绍如何扩展MiniOB的SQL解析器，并对词法分析和语法分析文件做一些简单的介绍。

# 词法分析

> 词法分析文件 lex_sql.l

词法分析的基本功能是读取文件或字符串的一个一个字符，然后按照特定的模式(pattern)去判断是否匹配，然后输出一个个的token。比如对于 `CALC 1+2`，词法分析器会输出 `CALC`、`1`、`+`、`2`、`EOF`。

扩展功能时，我们最关心的是词法分析的模式如何编写以及flex如何去执行模式解析的。
在lex_sql.l文件中，我们可以看到这样的代码：
```
[\-]?{DIGIT}+  yylval->number=atoi(yytext); RETURN_TOKEN(NUMBER);
HELP           RETURN_TOKEN(HELP);
DESC           RETURN_TOKEN(DESC);
CREATE         RETURN_TOKEN(CREATE);
DROP           RETURN_TOKEN(DROP);
```

每一行都是一个模式，左边是模式，使用正则表达式编写，右边是我们返回的token，这里的token是枚举类型，是我们在yacc_sql.y中定义的。

`[\-]?{DIGIT}+` 就是一个表示数字的正则表达式。
`HELP` 表示完全匹配"HELP" 字符串。

flex 在匹配这些模式时有一些规则，如果输出的结果与自己的预期不符，可以使用这些规则检查一下，是否自己写的模式和模式中的顺序是否符合这些规则：
- 优先匹配最长的模式
- 如果有多个模式匹配到同样长的字符串，那么优先匹配在文件中靠前的模式

另外还有一些词法分析的知识点，都在lex_sql.l中加了注释，不多做赘述，在这里罗列提示一下。
- flex 根据编写的 .l 规则文件，和指定的命令生成.c代码。不过miniob把.c文件改成了.cpp后缀；
- `%top{}` 的代码会放在flex生成的代码最开头的地方；
- `%{ %}` 中的代码会被移动到flex生成的代码中；
- flex提供了一些选项，可以在命令行中指定，也可以在.l文件中，使用 %option指定；
- flex 中编写模式时，有一些变量是预定义的，yylval就是返回值，可以认为是yacc中使用%union定义的结构; yytext 是解析的当前token的字符串，yyleng 是token的长度，yycolumn 是当前的列号，yylineno 是当前行号；
- 如果需要每个token在原始文本中的位置，可以使用宏定义 YY_USER_ACTION，但是需要自己编写代码记录才能传递给yacc；
- `%% %%`之间的代码是模式匹配代码，之后的代码会被复制到生成代码的最后。

# 语法分析
> 语法分析文件 yacc_sql.y

语法分析的基本功能是根据词法分析的结果，按照语法规则，生成语法树。

与词法分析类似，语法分析工具(这里使用的是bison)也会根据我们编写的.y规则文件，生成.c代码，miniob 这里把.c代码改成了.cpp代码，因此.y文件中可以使用c++的语法和标准库。

类似词法分析，我们在扩展语法分析时，最关心的也只有规则编写的部分。

在yacc_sql.y文件中，我们可以看到这样的代码：

```yacc
expression_list:
    expression
    {
      $$ = new std::vector<Expression*>;
      $$->emplace_back($1);
    }
    | expression COMMA expression_list {
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new std::vector<Expression *>;
      }
      $$->emplace_back($1);
    }
    ;
```

这个规则描述表达式列表的语法，表达式列表可以是单个表示，或者"单个表达式 逗号 表达式列表"的形式，第二个规则是一个递归的定义。
可以看到，多个规则模式描述使用 "|" 分开。

为了方便说明，我这里再换一个语句：
```yacc
create_table_stmt:    /*create table 语句的语法解析树*/
    CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE
    {
      $$ = new ParsedSqlNode(SCF_CREATE_TABLE);
      CreateTableSqlNode &create_table = $$->create_table;
      create_table.relation_name = $3;
      free($3);

      std::vector<AttrInfoSqlNode> *src_attrs = $6;

      if (src_attrs != nullptr) {
        create_table.attr_infos.swap(*src_attrs);
      }
      create_table.attr_infos.emplace_back(*$5);
      std::reverse(create_table.attr_infos.begin(), create_table.attr_infos.end());
      delete $5;
    }
    ;
```

每个规则描述，比如`create_table_stmt` 都会生成一个结果，这个结果在.y中以 "$$" 表示，某个语法中描述的各个token，按照顺序可以使用 "$1 $2 $3" 来引用，比如`ID` 就是 $3， attr_def 是 $5。"$n" 的类型都是 `YYSTYPE`。YYSTYPE 是bison根据.y生成的类型，对应我们的规则文件就是 %union，YYSTYPE 也是一个union结构。比如 我们在.y文件中说明 `%type <sql_node> create_table_stmt`，表示 create_table_stmt 的类型对应了 %union 中的成员变量 sql_node。我们在 %union 中定义了 `ParsedSqlNode * sql_node;`，那么 create_table_stmt 的类型就是 `ParsedSqlNode *`，对应了 YYSTYPE.sql_node。

%union 中定义的数据类型，除了简单类型，大部分是在parse_defs.h中定义的，表达式Expression是在expression.h中定义的，Value是在value.h中定义的。

由于在定义语法规则时，这里都使用了左递归，用户输入的第一个元素会放到最前面，因此在计算得出最后的结果时，我们需要将列表（这里很多使用vector记录）中的元素逆转一下。

**语法分析中如何使用位置信息**

首先我们要在.y文件中增加%locations，告诉bison我们需要位置信息。其次需要与词法分析相配合，需要词法分析返回一个token时告诉语法分析此token的位置信息，包括行号、token起始位置和结束位置。与访问规则结果 $$ 类似，访问某个元素的位置信息可以使用 @$、 @1、@2 等。

词法分析中，yylineno记录当前是第几行，不过MiniOB当前没有处理多行文本。yycolumn记录当前在第几列，yyleng记录当前token文本的长度。词法分析提供了宏定义 `YY_USER_INIT` 可以在每次解析之前执行一些代码，而 `YY_USER_ACTION` 宏在每次解析完token后执行的代码。我们可以在`YY_USER_INIT`宏中对列号进行初始化，然后在`YY_USER_ACTION`计算token的位置信息，然后使用 `yylloc` 将位置信息传递给语法分析。而在引用 @$ 时，就是引用了 yylloc。

> 这些宏定义和位置的引用可以参考lex_sql.l和yacc_sql.y，搜索相应的关键字即可。

如果觉得位置信息传递和一些特殊符号 $$或 @$ 等使用感到困惑，可以直接看 flex和bison生成的代码，会让自己理解的更清晰。

# 如何编译词法分析和语法分析模块

在 src/observer/sql/parser/ 目录下，执行以下命令：

```shell
./gen_parser.sh
```

将会生成词法分析代码 lex_sql.h 和 lex_sql.cpp，语法分析代码 yacc_sql.hpp 和 yacc_sql.cpp。

注意：flex 使用 2.5.35 版本测试通过，bison使用**3.7**版本测试通过(请不要使用旧版本，比如macos自带的bision）。

注意：当前没有把lex_sql.l和yacc_sql.y加入CMakefile.txt中，所以修改这两个文件后，需要手动生成c代码，然后再执行编译。

如果使用visual studio code，可以直接选择 "终端/Terminal" -> "Run task..." -> "gen_parser"，即可生成代码。

# 如何调试词法分析和语法分析模块

代码中可以直接使用日志模块打印日志，对于yacc_sql.y也可以直接使用gdb调试工具调试。

# 参考
- 《flex_bison》  flex/bison手册
- [flex开源源码](https://github.com/westes/flex)
- [bison首页](https://www.gnu.org/software/bison/)