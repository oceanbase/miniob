# MemTracer

MemTracer 被用于监控 MiniOB 内存使用；用于在内存受限的条件下，运行和调试 MiniOB。MemTracer 通过 hook 内存分配释放函数，记录 MiniOB 进程中的内存分配情况。
## 原理介绍

## 使用介绍
### 编译
可通过指定 `WITH_MEMTRACER` 控制 MemTracer 的编译（默认编译），MemTracer 动态库默认输出在`${CMAKE_BINARY_DIR}/lib` 目录下。下述示例将关闭MemTracer 的编译。
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
```
MT_MEMORY_LIMIT=100000000 LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```
当由于申请内存超过限额退出，则在退出时会打印相关日志：
```
[MEMTRACER] alloc memory:24, allocated_memory: 31653580, memory_limit: 31653600, Memory limit exceeded!
```
2. 通过链接 MemTracer，可以在 MiniOB 进程的任意位置获取当前内存使用情况。

步骤1: 链接 libmemtracer.so 到 MiniOB 进程中。

步骤2: 在需要获取内存使用情况的位置，调用 `memtracer::allocated_memory()` 函数，获取当前内存使用情况。

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
