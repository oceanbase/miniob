/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// STL (Standard Template Library) 是C++标准库的核心组件，提供了一组通用的模板类和函数，
// 用于实现常用的数据结构和算法。STL极大地提高了C++编程的效率和代码质量。
//
// STL主要由三个核心组件组成：
// 1. 容器(Containers) - 用于存储数据的模板类，如vector, list, map, set等。
//    - 序列容器：vector, list, deque - 以线性方式存储元素
//    - 关联容器：map, set, multimap, multiset，unordered_map, unordered_set - 以键值方式存储和快速查找元素
//    - 容器适配器：stack, queue, priority_queue - 提供特定接口的容器包装器
//
// 2. 迭代器(Iterators) - 用于遍历容器元素的类似指针的对象，提供容器与算法之间的桥梁。
//    - 输入/输出迭代器：用于单向遍历和读/写操作
//    - 前向迭代器：支持多次遍历和读写操作
//    - 双向迭代器：允许向前和向后遍历
//    - 随机访问迭代器：支持随机访问和完整的迭代器运算（如加减整数进行跳转、迭代器间距离计算、比较运算等）
//
// 3. 算法(Algorithms) - 提供对容器内容进行操作的函数模板，如排序、搜索、变换等。
//    - 非修改序列算法：find, count, for_each等
//    - 修改序列算法：copy, transform, replace等
//    - 排序和相关操作：sort, merge, binary_search等
//    - 数值算法：accumulate, inner_product等
//
// STL的设计理念是泛型编程，通过模板实现与具体数据类型无关的通用代码，实现代码复用和提高效率。
// 它的强大之处在于将数据结构与算法分离，允许通过标准接口(迭代器)对不同容器应用相同的算法。

// container.cpp
// STL (Standard Template Library) 使用示例

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <queue>
#include <stack>
#include <string>
#include <algorithm>
#include <functional>
#include <numeric>
#include <memory>
#include <utility>
#include <iterator>

int main()
{
  std::cout << "===== C++ STL (Standard Template Library) Examples =====\n";

  // ===== 向量容器(vector)示例 =====
  std::cout << "===== Vector Container Examples =====\n";

  // 创建和初始化
  std::vector<int> vec1;                            // 空向量
  std::vector<int> vec2(5, 10);                     // 包含5个值为10的元素的向量
  std::vector<int> vec3 = {1, 2, 3, 4, 5};          // 使用初始化列表
  std::vector<int> vec4(vec3.begin(), vec3.end());  // 使用迭代器初始化

  // 基本操作
  std::cout << "Vector size: " << vec3.size() << std::endl;
  std::cout << "Vector capacity: " << vec3.capacity() << std::endl;
  std::cout << "First element: " << vec3.front() << std::endl;
  std::cout << "Last element: " << vec3.back() << std::endl;
  std::cout << "Element at index 2: " << vec3[2] << std::endl;

  // 添加和删除元素
  vec1.push_back(10);
  vec1.push_back(20);
  vec1.push_back(30);
  std::cout << "Vector after push_back: ";
  for (const auto &elem : vec1) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  vec1.pop_back();  // 删除最后一个元素
  std::cout << "Vector after pop_back: ";
  for (const auto &elem : vec1) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 插入和擦除
  vec3.insert(vec3.begin() + 2, 42);  // 在索引2处插入值42
  std::cout << "Vector after insertion: ";
  for (const auto &elem : vec3) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  vec3.erase(vec3.begin() + 2);  // 删除索引2处的元素
  std::cout << "Vector after erasure: ";
  for (const auto &elem : vec3) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 调整大小
  vec1.resize(5, 99);  // 调整为5个元素，新元素为99
  std::cout << "Vector after resizing: ";
  for (const auto &elem : vec1) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 清空
  vec1.clear();
  std::cout << "Is vector empty after clearing: " << (vec1.empty() ? "Yes" : "No") << std::endl;

  // 使用reserve避免重新分配
  vec1.reserve(100);  // 预留100个元素的空间
  std::cout << "Capacity after reserve: " << vec1.capacity() << std::endl;

  // ===== 链表容器(list)示例 =====
  std::cout << "===== List Container Examples =====\n";

  // 创建和初始化
  std::list<int> list1;                    // 空链表
  std::list<int> list2(5, 10);             // 包含5个值为10的元素的链表
  std::list<int> list3 = {1, 2, 3, 4, 5};  // 使用初始化列表

  // 基本操作
  std::cout << "List size: " << list3.size() << std::endl;
  std::cout << "First element: " << list3.front() << std::endl;
  std::cout << "Last element: " << list3.back() << std::endl;

  // 添加和删除元素
  list1.push_back(10);
  list1.push_back(20);
  list1.push_front(5);  // 在链表前端添加元素
  std::cout << "List after adding elements: ";
  for (const auto &elem : list1) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  list1.pop_back();
  list1.pop_front();
  std::cout << "List after removing front and back elements: ";
  for (const auto &elem : list1) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 插入和删除
  auto it = list3.begin();
  std::advance(it, 2);  // 移动迭代器到第三个位置
  list3.insert(it, 42);
  std::cout << "List after insertion: ";
  for (const auto &elem : list3) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  it = list3.begin();
  std::advance(it, 2);
  list3.erase(it);
  std::cout << "List after erasure: ";
  for (const auto &elem : list3) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 特殊操作
  std::list<int> list4 = {10, 20, 30, 40};
  list3.splice(list3.end(), list4);  // 将list4的所有元素移动到list3末尾
  std::cout << "List3 after splicing: ";
  for (const auto &elem : list3) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  list3.sort();  // 链表排序
  std::cout << "List after sorting: ";
  for (const auto &elem : list3) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  list3.unique();  // 移除连续的重复元素
  std::cout << "List after removing duplicates: ";
  for (const auto &elem : list3) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  list3.reverse();  // 反转链表
  std::cout << "List after reversing: ";
  for (const auto &elem : list3) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // ===== 映射容器(map)示例 =====
  std::cout << "===== Map Container Examples =====\n";

  // 创建和初始化
  std::map<std::string, int> map1;
  std::map<std::string, int> map2 = {{"apple", 5}, {"banana", 8}, {"orange", 10}};

  // 插入和访问
  map1["one"]   = 1;
  map1["two"]   = 2;
  map1["three"] = 3;

  map1.insert({"four", 4});
  map1.insert(std::make_pair("five", 5));

  std::cout << "map1[\"two\"]: " << map1["two"] << std::endl;

  // 遍历映射
  std::cout << "Map1 contents: \n";
  for (const auto &pair : map1) {
    std::cout << pair.first << " => " << pair.second << std::endl;
  }

  // 查找和检查
  auto map_it = map2.find("banana");
  if (map_it != map2.end()) {
    std::cout << "Found: " << map_it->first << " => " << map_it->second << std::endl;
  }

  bool exists = map2.count("grape") > 0;
  std::cout << "Does grape exist? " << (exists ? "Yes" : "No") << std::endl;

  // 删除
  map2.erase("banana");
  std::cout << "Map2 size after erasure: " << map2.size() << std::endl;

  // unordered_map (哈希表实现)
  std::cout << "===== Unordered Map Container Examples =====\n";

  std::unordered_map<std::string, int> umap = {{"red", 1}, {"green", 2}, {"blue", 3}};

  umap["yellow"] = 4;

  std::cout << "Unordered map contents: \n";
  for (const auto &pair : umap) {
    std::cout << pair.first << " => " << pair.second << std::endl;
  }

  std::cout << "Bucket count: " << umap.bucket_count() << std::endl;
  std::cout << "Load factor: " << umap.load_factor() << std::endl;

  // ===== 集合容器(set)示例 =====
  std::cout << "===== Set Container Examples =====\n";

  // 创建和初始化
  std::set<int> set1;
  std::set<int> set2 = {5, 3, 1, 4, 2};

  // 插入
  set1.insert(10);
  set1.insert(20);
  set1.insert(30);
  set1.insert(10);  // 重复元素不会被插入

  std::cout << "Set1 size: " << set1.size() << std::endl;

  // 遍历集合
  std::cout << "Set2 contents (auto-sorted): ";
  for (const auto &elem : set2) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 查找和检查
  auto set_it = set1.find(20);
  if (set_it != set1.end()) {
    std::cout << "Found element: " << *set_it << std::endl;
  }

  exists = set1.count(50) > 0;
  std::cout << "Does 50 exist? " << (exists ? "Yes" : "No") << std::endl;

  // 删除
  set1.erase(10);
  std::cout << "Set1 contents after erasure: ";
  for (const auto &elem : set1) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // unordered_set (哈希表实现)
  std::cout << "===== Unordered Set Examples =====\n";

  std::unordered_set<std::string> uset = {"cat", "dog", "bird"};

  uset.insert("fish");

  std::cout << "Unordered set contents (unordered): ";
  for (const auto &elem : uset) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // ===== 其他容器示例 =====
  std::cout << "===== Other Container Examples =====\n";

  // 双端队列 (deque) - 两端都可以高效插入删除
  std::cout << "-- Deque Examples --\n";
  std::deque<int> deq = {1, 2, 3};

  deq.push_front(0);
  deq.push_back(4);

  std::cout << "Deque contents: ";
  for (const auto &elem : deq) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 栈 (stack) - 后进先出 (LIFO)
  std::cout << "-- Stack Examples --\n";
  std::stack<int> stack;

  stack.push(1);
  stack.push(2);
  stack.push(3);

  std::cout << "Stack top element: " << stack.top() << std::endl;

  stack.pop();
  std::cout << "Stack top after pop: " << stack.top() << std::endl;

  // 队列 (queue) - 先进先出 (FIFO)
  std::cout << "-- Queue Examples --\n";
  std::queue<int> queue;

  queue.push(1);
  queue.push(2);
  queue.push(3);

  std::cout << "Queue front: " << queue.front() << std::endl;
  std::cout << "Queue back: " << queue.back() << std::endl;

  queue.pop();
  std::cout << "Queue front after pop: " << queue.front() << std::endl;

  // 优先队列 (priority_queue) - 最大值优先
  std::cout << "-- Priority Queue Examples --\n";
  std::priority_queue<int> pq;

  pq.push(10);
  pq.push(30);
  pq.push(20);

  std::cout << "Priority queue top element (max): " << pq.top() << std::endl;

  pq.pop();
  std::cout << "Top element after pop: " << pq.top() << std::endl;

  // 自定义比较的最小优先队列
  std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;

  min_pq.push(10);
  min_pq.push(30);
  min_pq.push(20);

  std::cout << "Min priority queue top element (min): " << min_pq.top() << std::endl;

  // ===== 迭代器使用模式 =====
  std::cout << "===== Iterator Usage Patterns =====\n";

  std::vector<int> vec = {10, 20, 30, 40, 50};

  // 基本迭代器用法
  std::cout << "Traversing vector with iterators: ";
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    std::cout << *it << " ";
  }
  std::cout << std::endl;

  // 反向迭代器
  std::cout << "Traversing vector with reverse iterators: ";
  for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
    std::cout << *rit << " ";
  }
  std::cout << std::endl;

  // 常量迭代器
  std::cout << "Traversing vector with const iterators: ";
  for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
    // *it = 100; // 错误：不能通过常量迭代器修改
    std::cout << *it << " ";
  }
  std::cout << std::endl;

  // 迭代器算术
  auto iter_it = vec.begin();
  std::advance(iter_it, 2);  // 前进2个位置
  std::cout << "Element at advance(begin, 2): " << *iter_it << std::endl;

  auto next_it = std::next(iter_it);  // 返回下一个位置的迭代器
  std::cout << "Element at next iterator: " << *next_it << std::endl;

  auto prev_it = std::prev(iter_it);  // 返回前一个位置的迭代器
  std::cout << "Element at prev iterator: " << *prev_it << std::endl;

  // 迭代器范围
  auto distance = std::distance(vec.begin(), vec.end());
  std::cout << "Distance from begin() to end(): " << distance << std::endl;

  // 使用迭代器适配器
  std::cout << "Iterator adapters examples:\n";

  // 插入迭代器
  std::vector<int> dest;
  std::copy(vec.begin(), vec.end(), std::back_inserter(dest));

  std::cout << "Destination vector after copy with back_inserter: ";
  for (const auto &elem : dest) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 流迭代器
  std::cout << "Vector output using ostream_iterator: ";
  std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(std::cout, " "));
  std::cout << std::endl;

  // ===== 算法库示例 =====
  std::cout << "===== Algorithm Library Examples =====\n";

  std::vector<int> alg_vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};
  std::vector<int> alg_vec2(9);

  // 排序算法
  std::cout << "-- Sorting and Searching --\n";

  // 排序
  std::sort(alg_vec.begin(), alg_vec.end());
  std::cout << "Vector after sorting: ";
  for (const auto &elem : alg_vec) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 二分查找
  bool found = std::binary_search(alg_vec.begin(), alg_vec.end(), 7);
  std::cout << "Binary search for 7: " << (found ? "Found" : "Not found") << std::endl;

  // 查找
  auto find_it = std::find(alg_vec.begin(), alg_vec.end(), 4);
  if (find_it != alg_vec.end()) {
    std::cout << "Found 4 at index " << std::distance(alg_vec.begin(), find_it) << std::endl;
  }

  // 最大最小值
  auto [min_it, max_it] = std::minmax_element(alg_vec.begin(), alg_vec.end());
  std::cout << "Min value: " << *min_it << ", Max value: " << *max_it << std::endl;

  // 修改序列算法
  std::cout << "-- Modifying Sequence Algorithms --\n";

  // 复制
  std::copy(alg_vec.begin(), alg_vec.end(), alg_vec2.begin());
  std::cout << "alg_vec2 after copy: ";
  for (const auto &elem : alg_vec2) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 填充
  std::fill(alg_vec2.begin() + 3, alg_vec2.begin() + 6, 99);
  std::cout << "alg_vec2 after fill: ";
  for (const auto &elem : alg_vec2) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 替换
  std::replace(alg_vec.begin(), alg_vec.end(), 4, 44);
  std::cout << "alg_vec after replace: ";
  for (const auto &elem : alg_vec) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 转换
  std::vector<int> alg_vec3(alg_vec.size());
  std::transform(alg_vec.begin(), alg_vec.end(), alg_vec3.begin(), [](int n) { return n * 2; });
  std::cout << "alg_vec3 after transform (each element * 2): ";
  for (const auto &elem : alg_vec3) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  // 非修改序列算法
  std::cout << "-- Non-modifying Sequence Algorithms --\n";

  // 计数
  int count = std::count_if(alg_vec3.begin(), alg_vec3.end(), [](int n) { return n > 10; });
  std::cout << "Number of elements > 10 in alg_vec3: " << count << std::endl;

  // 所有元素满足条件？
  bool all = std::all_of(alg_vec.begin(), alg_vec.end(), [](int n) { return n < 100; });
  std::cout << "Are all elements in alg_vec < 100? " << (all ? "Yes" : "No") << std::endl;

  // 任意元素满足条件？
  bool any = std::any_of(alg_vec.begin(), alg_vec.end(), [](int n) { return n > 8; });
  std::cout << "Are any elements in alg_vec > 8? " << (any ? "Yes" : "No") << std::endl;

  // 数值算法
  std::cout << "-- Numeric Algorithms --\n";

  // 累加
  int sum = std::accumulate(alg_vec.begin(), alg_vec.end(), 0);
  std::cout << "Sum of elements in alg_vec: " << sum << std::endl;

  // 内积
  int product = std::inner_product(alg_vec.begin(), alg_vec.end(), alg_vec3.begin(), 0);
  std::cout << "Inner product of alg_vec and alg_vec3: " << product << std::endl;

  // 部分和
  std::vector<int> sums(alg_vec.size());
  std::partial_sum(alg_vec.begin(), alg_vec.end(), sums.begin());
  std::cout << "Partial sums of alg_vec: ";
  for (const auto &elem : sums) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  return 0;
}