---
title: MemTracer 内存监控
---
# MemTracer

MemTracer 是一个动态链接库，被用于监控 MiniOB 内存使用；用于在内存受限的条件下，运行和调试 MiniOB。MemTracer 通过 hook 内存分配释放函数，记录 MiniOB 进程中的内存分配情况。
## 原理介绍
MemTracer 对内存分配释放函数进行了覆盖（override），以达到对内存动态分配释放（如`malloc/free`, `new/delete`）的监控。除此之外，MemTracer 还会将 MiniOB 进程中代码段等内存占用统计在内。

通过在 `LD_PRELOAD` 环境变量中指定 MemTracer 动态库来覆盖 glibc 中的符号，可以实现可插拔方式监控 MiniOB 进程的内存占用。

MemTracer 支持设置最大内存限额，当 MiniOB 进程申请超过内存限额的内存时，MemTracer 会调用 `exit(-1)` 使 MiniOB 进程退出。

## 使用介绍
### 编译
可通过指定 `WITH_MEMTRACER` 控制 MemTracer 的编译（默认编译），MemTracer 动态库默认输出在`${CMAKE_BINARY_DIR}/lib` 目录下。
下述示例将关闭MemTracer 的编译。
```
sudo bash build.sh init
bash build.sh release -DWITH_MEMTRACER=OFF
```
### 运行
通过指定 `LD_PRELOAD` 环境变量， 将 MemTracer 动态库加载到 MiniOB 进程中。如：
```
LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```
通过指定`MT_PRINT_INTERVAL_MS` 环境变量，设置内存使用的打印间隔时间，单位为毫秒（ms），默认为 5000 ms（5s）。通过指定`MT_MEMORY_LIMIT` 环境变量，设置内存使用的上限，单位为字节，当超过该值，MiniOB 进程会立即退出。下述示例表明设置内存使用情况的打印间隔为 1000 ms（1s），内存使用上限为1000 字节。
```
MT_PRINT_INTERVAL_MS=1000 MT_MEMORY_LIMIT=1000 LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```
### 使用场景示例
1. 通过指定 MiniOB 进程的最大内存限额，可以模拟在内存受限的情况下运行、调试 MiniOB。当超出最大内存限额后，MiniOB 进程会自动退出。

指定最大内存限额：
```
MT_MEMORY_LIMIT=100000000 LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```
当由于申请内存超过限额退出，则在退出时会打印相关日志：
```
[MEMTRACER] alloc memory:24, allocated_memory: 31653580, memory_limit: 31653600, Memory limit exceeded!
```
2. 通过链接 MemTracer，可以在 MiniOB 进程的任意位置获取当前内存使用情况。

步骤1: 链接 libmemtracer.so 到 MiniOB 进程中。

步骤2: 在需要获取内存使用情况的位置，调用 `memtracer/mt_info.h` 头文件中的 `memtracer::allocated_memory()` 函数，获取当前内存使用情况。

示例代码如下：

```
diff --git a/src/observer/CMakeLists.txt b/src/observer/CMakeLists.txt
index c62ac3c..d105fbf 100644
--- a/src/observer/CMakeLists.txt
+++ b/src/observer/CMakeLists.txt
@@ -20,7 +20,7 @@ FIND_PACKAGE(Libevent CONFIG REQUIRED)
 
 # JsonCpp cannot work correctly with FIND_PACKAGE
 
-SET(LIBRARIES common pthread dl libevent::core libevent::pthreads libjsoncpp.a)
+SET(LIBRARIES common pthread dl libevent::core libevent::pthreads libjsoncpp.a memtracer)
 
 # 指定目标文件位置
 SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
diff --git a/src/observer/sql/parser/parse.cpp b/src/observer/sql/parser/parse.cpp
index def0ed1..4f22799 100644
--- a/src/observer/sql/parser/parse.cpp
+++ b/src/observer/sql/parser/parse.cpp
@@ -15,6 +15,7 @@ See the Mulan PSL v2 for more details. */
 #include "sql/parser/parse.h"
 #include "common/log/log.h"
 #include "sql/expr/expression.h"
+#include "memtracer/mt_info.h"
 
 RC parse(char *st, ParsedSqlNode *sqln);
 
@@ -41,6 +42,7 @@ int sql_parse(const char *st, ParsedSqlResult *sql_result);
 
 RC parse(const char *st, ParsedSqlResult *sql_result)
 {
+  LOG_ERROR("parse sql `%s`, allocated: %lu\n", st, memtracer::allocated_memory());
   sql_parse(st, sql_result);
   return RC::SUCCESS;
 }
```
### 注意
1. MemTracer 会记录 `mmap` 映射的整个虚拟内存占用, 因此不建议使用 `mmap` 管理内存。
2. 不允许使用绕过常规内存分配（`malloc`/`free`, `new`/`delete`）的方式申请并使用内存。如使用 `brk/sbrk/syscall` 等。
3. MemTracer 不支持与 sanitizers （ASAN等）一起使用。
4. MemTracer 除统计动态内存的申请释放，也会将进程的代码段等内存占用统计在内。
5. 引入 MemTracer 后，会对内存申请释放函数的性能产生一定影响，在 Github Action 中测试结果如下：
```
// MemTracer 开启
-----------------------------------------------------------------------------------
Benchmark                         Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------
BM_MallocFree/8                42.1 ns         42.1 ns     17090212 bytes_per_second=181.357M/s
BM_MallocFree/64               39.9 ns         39.9 ns     16763826 bytes_per_second=1.49326G/s
BM_MallocFree/512              40.1 ns         40.1 ns     17553391 bytes_per_second=11.8925G/s
BM_MallocFree/1024             39.9 ns         39.9 ns     17548820 bytes_per_second=23.9052G/s
BM_MallocFree/1048576          53.4 ns         53.4 ns     13150036 bytes_per_second=17.8628T/s
BM_MallocFree/8388608          53.3 ns         53.3 ns     13155704 bytes_per_second=143.106T/s
BM_MallocFree/1073741824      24877 ns        24750 ns        28814 bytes_per_second=39.4565T/s
BM_NewDelete/8                 40.6 ns         40.6 ns     17270523 bytes_per_second=187.866M/s
BM_NewDelete/64                40.5 ns         40.5 ns     17279131 bytes_per_second=1.47047G/s
BM_NewDelete/512               40.5 ns         40.5 ns     17276789 bytes_per_second=11.7686G/s
BM_NewDelete/1024              40.5 ns         40.5 ns     17279484 bytes_per_second=23.5455G/s
BM_NewDelete/1048576           53.8 ns         53.8 ns     13003320 bytes_per_second=17.7138T/s
BM_NewDelete/8388608           54.0 ns         54.0 ns     13011425 bytes_per_second=141.183T/s
BM_NewDelete/1073741824       24844 ns        24611 ns        29271 bytes_per_second=39.6796T/s
// MemTracer 关闭
-----------------------------------------------------------------------------------
Benchmark                         Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------
BM_MallocFree/8                10.1 ns         10.1 ns     70194674 bytes_per_second=753.492M/s
BM_MallocFree/64               10.8 ns         10.8 ns     64456192 bytes_per_second=5.50213G/s
BM_MallocFree/512              10.9 ns         10.9 ns     64672143 bytes_per_second=43.8294G/s
BM_MallocFree/1024             10.8 ns         10.8 ns     64616114 bytes_per_second=88.0599G/s
BM_MallocFree/1048576          23.5 ns         23.5 ns     29758318 bytes_per_second=40.5388T/s
BM_MallocFree/8388608          23.5 ns         23.5 ns     29743010 bytes_per_second=324.33T/s
BM_MallocFree/1073741824      20574 ns        20436 ns        34738 bytes_per_second=47.7856T/s
BM_NewDelete/8                 13.4 ns         13.4 ns     50775388 bytes_per_second=570.14M/s
BM_NewDelete/64                14.2 ns         14.2 ns     49231328 bytes_per_second=4.18804G/s
BM_NewDelete/512               14.3 ns         14.3 ns     49198936 bytes_per_second=33.4308G/s
BM_NewDelete/1024              14.3 ns         14.3 ns     49215085 bytes_per_second=66.7825G/s
BM_NewDelete/1048576           26.9 ns         26.9 ns     25987611 bytes_per_second=35.4036T/s
BM_NewDelete/8388608           26.9 ns         26.9 ns     25997689 bytes_per_second=283.11T/s
BM_NewDelete/1073741824       20694 ns        20648 ns        34975 bytes_per_second=47.2954T/s
```
