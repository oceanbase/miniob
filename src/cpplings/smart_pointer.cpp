/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// 智能指针是一种用于内存管理的数据结构，它表现得像指针，但是
// 它还包含了一些额外的信息，例如指针的引用计数。这些额外的信息
// 使得智能指针可以自动处理内存分配和释放，从而减少内存泄漏的风险。
// 在 C++ 中，std::unique_ptr 和 std::shared_ptr 是被广泛使用的智能指针。
// std::unique_ptr 和 std::shared_ptr 自动处理内存分配和释放。
// 标准库提供的这两种智能指针的区别在于管理底层指针的方法不同：
// shared_ptr允许多个指针指向同一个对象；
// unique_ptr则“独占”所指向的对象。

#include <iostream>
#include <memory>

class Foo
{
public:
  Foo()
  {
    // TODO: 添加必要的日志信息，观察函数何时调用。
  }

  ~Foo()
  {
    // TODO: 添加必要的日志信息，观察函数何时调用。
  }

  void display() { std::cout << "Displaying Foo content." << std::endl; }
};

int main()
{
  // 使用 std::unique_ptr
  {
    // std::make_unique 是 C++14 引入的，C++11 可以使用 std::unique_ptr<Foo> uni_ptr(new Foo());
    std::unique_ptr<Foo> uni_ptr = std::make_unique<Foo>();
    uni_ptr->display();
    std::cout << "uni_ptr block ended." << std::endl;
  }
  // unique_ptr 超出作用域，自动销毁对象，调用 Foo 的析构函数
  std::cout << "uni_ptr destroy" << std::endl;

  // 使用 std::shared_ptr
  {
    std::shared_ptr<Foo> shared_ptr1 = std::make_shared<Foo>();
    {
      std::shared_ptr<Foo> shared_ptr2 = shared_ptr1;  // 增加引用计数
      shared_ptr2->display();
      std::cout << "shared_ptr use_count(): " << shared_ptr2.use_count() << std::endl;
      // sharedPtr2 超出作用域，引用计数减少但不销毁对象
    }
    std::cout << "shared_ptr use_count(): " << shared_ptr1.use_count() << std::endl;
    // sharedPtr1 超出作用域，引用计数减少到0，自动销毁对象
    std::cout << "shared_tr block ended." << std::endl;
  }

  std::cout << "shared_tr destroyed" << std::endl;
  std::cout << "passed!" << std::endl;
  return 0;
}