# MemTracer

MemTracer 被用于监控 MiniOB 内存使用；用于在内存受限的条件下，运行和调试 MiniOB。MemTracer 通过 hook 内存分配释放函数，记录 MiniOB 进程中的内存分配情况。
## 使用介绍
### 编译
可通过指定 `WITH_MEMTRACER` 控制 MemTracer 的编译（默认编译），MemTracer 动态库默认输出在`${CMAKE_BINARY_DIR}/lib` 目录下。
```
sudo bash build.sh init
bash build.sh release -DWITH_MEMTRACER=OFF
```
### 运行
通过指定 `LD_PRELOAD` 环境变量， 将 MemTracer 动态库加载到 MiniOB 进程中。如：
```
LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```
通过指定`MT_PRINT_INTERVAL` 环境变量，设置内存使用的打印间隔时间，单位为微秒（ms），通过指定`MT_MEMORY_LIMIT` 环境变量，设置内存使用的上限，单位为字节，超过该值，MiniOB 进程会退出。下述示例表明设置内存使用情况的打印间隔为1000000ms（1s），内存使用上限为1000 字节。
```
MT_PRINT_INTERVAL=1000000 MT_MEMORY_LIMIT=1000 LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```
### 与 ASAN 一起使用
ASAN 是 Address Sanitizer 的缩写，是一个检测内存错误的工具。ASAN 可以检测到一些常见的内存错误，包括缓冲区溢出、内存泄漏等。可通过如下方式与 MemTracer 配合使用。
1. 编译。注意：确保 `ENABLE_ASAN=ON` 且 `STATIC_STDLIB=OFF` 且 `WITH_MEMTRACER=ON`（默认配置）。
```
sudo bash build.sh init
bash build.sh debug
```
2. 检查 MiniOB 已动态链接 ASAN。
```
ldd ./build_debug/bin/observer 
        libasan.so.5 => /lib64/libasan.so.5
```
3. 设置 ASAN 相关环境变量。
```
export ASAN_OPTIONS=verify_asan_link_order=0
``` 
4. 启动 MiniOB 进程。
```
LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```
### 使用场景示例
1. 通过指定MiniOB 进程的最大内存限额，可以模拟在内存受限的情况下运行、调试 MiniOB。当超出最大内存限额后，MiniOB 进程会自动退出。
```
MT_MEMORY_LIMIT=100000000 LD_PRELOAD=./lib/libmemtracer.so ./bin/observer
```
2. 通过链接 MemTracer，可以在 MiniOB 代码任意位置获取当前内存使用情况。示例可参考：`unittest/memtracer_test.cpp`.
### 注意
1. MemTracer 会记录 `mmap` 映射的整个虚拟内存占用, 因此不建议使用 `mmap` 管理内存。
2. 不允许使用绕过常规内存分配（`malloc`/`free`, `new`/`delete`）的方式申请并使用内存。
3. ASAN 与 MemTracer 的组合使用未经过充分测试，目前已知在 `ubuntu22.04` 上无法组合使用。
