> 这部分内容会介绍一些如何对miniob中的词法语法分析模块进行开发与调试，以及依赖的工具。

# 简介
SQL 解析分为词法分析与语法分析，与编译原理中介绍的类似。

# 如何编译词法分析和语法分析模块

在 src/observer/sql/parser/ 目录下，执行以下命令：

```shell
./gen_parser.sh
```

将会生成词法分析代码 lex_sql.h 和 lex_sql.cpp，语法分析代码 yacc_sql.hpp 和 yacc_sql.cpp。

注意：flex 使用 2.5.35 版本测试通过，bison使用**3.7**版本测试通过(请不要使用旧版本，比如macos自带的bision）。

注意：当前没有把lex_sql.l和yacc_sql.y加入CMakefile.txt中，所以修改这两个文件后，需要手动生成c代码，然后再执行编译。

# 如何调试词法分析和语法分析模块

代码中可以直接使用日志模块打印日志，对于yacc_sql.y也可以直接使用gdb调试工具调试。

# 如何手动安装bison
1. 下载一个合适版本的bison源码 [下载链接](http://ftp.gnu.org/gnu/bison/)，比如 [bison-3.7.tar.gz](http://ftp.gnu.org/gnu/bison/bison-3.7.tar.gz)

2. 在本地解压。`tar xzvf bison-3.7.tar.gz`，然后进入bison-3.7: `cd bison-3.7`;

3. 执行  `./configure --prefix="your/bison/install/path"`

4. 执行 make install

5. 安装完成

注意: 安装后的Bison在指定的安装目录的bin下，如果不调整PATH环境变量，无法直接使用到最新编译的bison二进制文件，需要写全路径使用，比如 your/bison/install/path/bin/bison。
