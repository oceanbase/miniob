这部分内容会介绍一些如何对miniob中的词法语法分析模块进行开发与调试，以及依赖的工具。

# 如何编译词法分析和语法分析模块

词法分析代码lex_sql.l使用下面的命令生成C代码：

```bash
flex --header-file=lex.yy.h lex_sql.l
```

生成 `lex.yy.c` 和`lex.yy.h`文件。

语法分析代码yacc_sql.y使用下面的命令生成C代码：

```bash
bison -d -b yacc_sql yacc_sql.y
```

将会生成代码yacc_sql.tab.c和yacc_sql.tab.h。

其中-b表示生成的源码文件前缀，-d表示生成一个头文件。

注意：flex 使用 2.5.35 版本测试通过，bison使用**3.7**版本测试通过(请不要使用旧版本，比如macos自带的bision）。

注意：当前没有把lex_sql.l和yacc_sql.y加入CMakefile.txt中，所以修改这两个文件后，需要手动生成c代码，然后再执行编译。

# 如何调试词法分析和语法分析模块

对于lex_sql.l，参考代码中的YYDEBUG宏，可以在lex_sql.l中增加调试代码和开启宏定义，也可以在编译时定义这个宏，比如直接修改lex.yy.c代码，在代码前面增加#define YYDEBUG 1。注意，lex.yy.c是自动生成代码，执行flex命令后，会把之前的修改覆盖掉。示例：

```c++
#include "yacc_sql.tab.h" 
extern int atoi(); 
extern double atof(); 
char * position = ""; 
#define YYDEBUG 1 // 可以在这里定义 YYDEBUG宏 
#ifdef YYDEBUG 
#define debug_printf  printf   // 也可以调整lex_sql.l代码，在定义YYDEBUG宏的时候，做更多事情 
#else 
#define debug_printf(...) 
#endif // YYDEBUG
```

对于yacc_sql.y，可以在yyerror中输出错误信息，或者直接使用调试工具设置断点跟踪。

# 如何手动安装bison
1. 下载一个合适版本的bison源码 [下载链接](http://ftp.gnu.org/gnu/bison/)，比如 [bison-3.7.tar.gz](http://ftp.gnu.org/gnu/bison/bison-3.7.tar.gz)

2. 在本地解压。`tar xzvf bison-3.7.tar.gz`，然后进入bison-3.7: `cd bison-3.7`;

3. 执行  `./configure --prefix="your/bison/install/path"`

4. 执行 make install

5. 安装完成

注意: 安装后的Bison在指定的安装目录的bin下，如果不调整PATH环境变量，无法直接使用到最新编译的bison二进制文件，需要写全路径使用，比如 your/bison/install/path/bin/bison。
