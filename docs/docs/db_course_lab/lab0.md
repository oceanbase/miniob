---
title: LAB#0 C++ 基础入门
---

> 请不要将代码提交到公开仓库（包括提交带有题解的 Pull Request），同时也请不要抄袭其他同学或网络上可能存在的代码。

# LAB#0 C++ 基础

MiniOB 是用 C++ 编写的，为了便于大家入门数据库系统实现原理与实践课程，这里提供了若干 C++ 编程特性相关的小练习（Cpplings），帮助大家熟悉一些 C++ 特性。一道简单的编程题目（实现 Bloom Filter），帮助大家熟悉 C++ 编程及 MiniOB 开发环境。其中 Cppings **不计入**实验成绩。

## C++ 小练习（Cpplings）
请参考 Cpplings: `src/cpplings/README.md`

## C++ 入门题目（Bloom Filter）

布隆过滤器（Bloom Filter）是1970年由伯顿·霍华德·布隆（Burton Howard Bloom）提出的。它实际上是一个很长的二进制向量和一系列随机映射函数。布隆过滤器可以用于检索一个元素是否在一个集合中。它的优点是空间效率和查询时间都远远超过一般的算法，缺点是有一定的误识别率和删除困难。

如果想判断一个元素是不是在一个集合里，一般想到的是将集合中所有元素保存起来，然后通过比较确定。链表、树、散列表（又叫哈希表，Hash table）等等数据结构都是这种思路。但是随着集合中元素的增加，我们需要的存储空间越来越大。同时检索速度也越来越慢。

布隆过滤器的原理是，当一个元素被加入集合时，通过K个散列函数将这个元素映射成一个位数组中的K个点，把它们置为1。检索时，我们只要看看这些点是不是都是1就（大约）知道集合中有没有它了：如果这些点有任何一个0，则被检元素一定不在；如果都是1，则被检元素很可能在。这就是布隆过滤器的基本思想。所以布隆过滤器可能会产生假阳性（误报，即不在集合中的元素被识别为在集合中），但不会产生假阴性（漏报，即在集合中的元素被识别为不在集合中）。

```
   ┌─┐             
 ┌─┼M┼───┐ ┌─┐     
 │ └─┴─┐ ├─┼T┼─┐   
 │     │ │ └─┤ │   
 │     │ │   │ │   
┌▼┬─┬─┬▼┬▼┬─┬▼┬▼┬─┐
│1│0│0│1│1│0│1│1│0│
└─┴─┴─┴─┴─┴─┴─┴─┴─┘
```

相比于其它的数据结构，布隆过滤器在空间和时间方面都有巨大的优势。布隆过滤器存储空间和插入/查询时间都是常数。另外，散列函数相互之间没有关系，方便由硬件并行实现。布隆过滤器不需要存储元素本身，在某些对保密要求非常严格的场合有优势。


### 任务

请完成 `src/oblsm/util/ob_bloomfilter.h` 中的 `ObBloomFilter` 类，实现布隆过滤器的功能。`ObBloomFilter` 中提供了必要的接口，请不要修改或删除这些接口，你可以添加任何有助于实现 `ObBloomFilter` 的成员函数和变量。

请实现下面代码中的：

- `ObBloomFilter::insert`：将一个元素插入布隆过滤器中，需要支持并发访问。
- `ObBloomFilter::clear`：清空布隆过滤器中的所有元素。
- `ObBloomFilter::contains`：判断一个元素是否在布隆过滤器中，需要支持并发访问。

```c++
class ObBloomfilter
{
public:
  ObBloomfilter(size_t hash_func_count = 4, size_t totoal_bits = 65536);

  void insert(const std::string &object)
  {
  }

  void clear()
  {
  }

  bool contains(const std::string &object) const
  {
    return true;
  }

private:
};

```

提示：你可以通过基于锁的方式来支持并发访问，也可以通过无锁的方式实现。

思考：布隆过滤器有哪些缺点，你能想到什么方法来改进/优化布隆过滤器存在的问题？

### 测试

可以通过运行 `unittest/oblsm/ob_bloomfilter_test.cpp` 来测试布隆过滤器的功能。

MiniOB 中的单测框架使用 [GTest](https://github.com/google/googletest)，在默认参数编译后，单测二进制程序位于 `$BUILD_DIR/bin/` 目录下，程序名与单测文件名对应。例如，`ob_bloomfilter_test.cpp` 对应的单测程序为 `$BUILD_DIR/bin/ob_bloomfilter_test`，通过运行该程序即可测试你的实现是否正确。

注意：如需运行单测，请移除单测名称中的 `DISABLED_` 前缀。

```bash
# 编译 miniob 代码
./build.sh debug --make
# 切换到 build 目录
cd build_debug/
# 运行单测程序
./bin/ob_bloomfilter_test
# 修改代码后重新编译可以在 build_debug 目录下运行 make 命令重新编译代码。
make
```