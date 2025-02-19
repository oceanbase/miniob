/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// 互斥锁（mutex）是一个用于控制对共享资源访问的同步原语。
// 当一个线程需要访问共享资源时，它会尝试锁定互斥锁。
// 如果互斥锁已经被其他线程锁定，请求线程将被阻塞，直到互斥锁被释放。
// 互斥锁的使用方法如下：
/*
  std::mutex m;
  m.lock(); // 获得锁
  ...
  m.unlock(); // 释放锁
*/
#include <iostream>  // std::cout
#include <atomic>    // std::atomic
#include <thread>    // std::thread
#include <vector>    // std::vector
#include <cassert>   // assert

struct Node
{
  int   value;
  Node *next;
};

std::atomic<Node *> list_head(nullptr);

// 向 `list_head` 中添加一个 value 为 `val` 的 Node 节点。
void append_node(int val)
{
  Node *old_head = list_head;
  Node *new_node = new Node{val, old_head};

  // TODO: 使用 mutex 来使这段代码线程安全。
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