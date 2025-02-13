
# cpplings

`cpplings` 包含一些关于 C++ 语言的代码练习，希望可以帮助大家学习和编写 C++ 代码，`cpplings` 的命名参考了 [rustlings](https://github.com/rust-lang/rustlings)。当然 C++ 语言的功能及特性过于庞大和复杂，cpplings 主要包含了在做 MiniOB 相关题目过程中可能用到的一些语法特性，并不能作为完整的 C++ 语言手册来学习。也非常欢迎大家提交 issue 或 PR 来贡献更多的练习题目。

在运行 `./build.sh debug -DENABLE_ASAN=OFF` 后，该目录下每个文件都会生成一个与其对应的同名的可执行文件（位于 `build_debug/bin/cpplings` 目录下），运行该可执行文件即可查看该代码的输出结果。

注意：由于 MiniOB 中会使用 ASAN 来检测内存错误，ASAN 的使用会导致代码执行变慢，练习中的例子无法很好的暴露出并发异常的问题，因此需要在编译时加上 ```-DENABLE_ASAN=OFF``` 来关闭 ASAN。也可以直接使用 g++ 来编译，例如对于 `lock.cpp` 可以通过 `g++ lock.cpp -o lock -g -lpthread` 来编译。

当你未实现文件中指定的 TODO 要求时，直接运行可执行文件会报错提示，例如：
```
$./bin/cpplings/condition_variable 
Printing count: 0
condition_variable: /myminiob/src/cpplings/condition_variable.cpp:48: void waiter_thread(): Assertion `count == expect_thread_num' failed.
Aborted
```

当你实现文件中要求的 TODO 后，运行可执行文件会输出部分日志信息以及 `passed!`，表示你已经通过了该测试。例如：
```
$./bin/cpplings/cas 
 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34 33 32 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
50
passed!
```