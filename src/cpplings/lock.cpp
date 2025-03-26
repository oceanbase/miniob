/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

/*
lock_guard 是 C++11 中一个简单的 RAII（Resource Acquisition Is Initialization）风格的锁，
用于在作用域内自动管理互斥量的锁定和解锁。当 lock_guard 对象被创建时，它会自动锁定互斥量，
当对象离开作用域时，它会自动解锁互斥量。lock_guard 不支持手动锁定和解锁，也不支持条件变量。

unique_lock 是 C++11 中一个更灵活的锁，它允许手动锁定和解锁互斥量，以及与 condition_variable 一起使用。
与 lock_guard 类似，unique_lock 也是一个 RAII 风格的锁，当对象离开作用域时，它会自动解锁互斥量。
unique_lock 还支持 lock() 和 unlock() 等操作。

scoped_lock 是 C++17 引入的一个锁，用于同时锁定多个互斥量，以避免死锁。
scoped_lock 是一个 RAII 风格的锁，当对象离开作用域时，它会自动解锁所有互斥量。
scoped_lock 不支持手动锁定和解锁，也不支持条件变量。
它的主要用途是在需要同时锁定多个互斥量时提供简单且安全的解决方案。
 */

#include <iostream>  // std::cout
#include <thread>    // std::thread
#include <vector>    // std::vector
#include <cassert>   // assert

struct Node
{
  int   value;
  Node *next;
};

Node *list_head(nullptr);

// 向 `list_head` 中添加一个 value 为 `val` 的 Node 节点。
void append_node(int val)
{
  Node *old_head = list_head;
  Node *new_node = new Node{val, old_head};

  // TODO: 使用 scoped_lock/unique_lock 来使这段代码线程安全。
  list_head = new_node;
}

int main()
{
  std::vector<std::thread> threads;
  int                      thread_num = 50;
  for (int i = 0; i < thread_num; ++i)
    threads.push_back(std::thread(append_node, i));
  for (auto &th : threads)
    th.join();

  // 注意：在 `append_node` 函数是线程安全的情况下，`list_head` 中将包含 50 个 Node 节点。
  int cnt = 0;
  for (Node *it = list_head; it != nullptr; it = it->next) {
    std::cout << ' ' << it->value;
    cnt++;
  }
  std::cout << '\n';
  assert(cnt == thread_num);
  std::cout << cnt << std::endl;

  Node *it;
  while ((it = list_head)) {
    list_head = it->next;
    delete it;
  }
  std::cout << "passed!" << std::endl;
  return 0;
}