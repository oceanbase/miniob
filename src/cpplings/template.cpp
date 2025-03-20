/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// 模板编程是C++中的一种强大特性，允许开发者编写与类型无关的代码。
// 模板可以应用于函数、类和变量，使开发者能够编写通用的算法和数据结构。
//
// C++模板主要分为函数模板和类模板两种：
// 函数模板允许创建可以处理多种数据类型的函数，而不必为每种类型编写单独的函数。
// 类模板允许创建可以存储和处理多种数据类型的通用类，例如STL中的容器。
// 模板特化允许为特定的类型提供专门的实现。
// 变参模板支持接受可变数量的参数，非常适用于构建递归数据结构和算法。
// SFINAE（Substitution Failure Is Not An Error）是一种模板编程技术，它允许在编译时根据类型特性选择正确的
// 函数重载或模板特化。当模板参数替换失败时，不会产生编译错误，而是简单地从重载解析
// 集合中删除该函数。
#include <iostream>
#include <type_traits>
#include <vector>
#include <string>

// 1. 基础函数模板
template <typename T>
T max_value(T a, T b)
{
  return (a > b) ? a : b;
}

// 2. 类模板
template <typename T>
class Container
{
private:
  T data;

public:
  Container(T value) : data(value) {}

  T get_data() const { return data; }

  void set_data(T value) { data = value; }
};

// 3. 模板特化 - 为std::string类型提供特殊实现
template <>
class Container<std::string>
{
private:
  std::string data;

public:
  Container(std::string value) : data(value) {}

  std::string get_data() const { return data; }

  void set_data(std::string value) { data = value; }

  // 字符串类型特有的方法
  std::size_t length() const { return data.length(); }
};

// 4. 变参模板 - 递归终止条件
template <typename T>
T sum(T value)
{
  return value;
}

// 变参模板 - 递归调用
template <typename T, typename... Args>
T sum(T first, Args... args)
{
  return first + sum(args...);
}

// 5. SFINAE技术示例
// SFINAE (Substitution Failure Is Not An Error) 是C++模板编程中的重要概念：
// - 它允许编译器在模板替换过程中，当某个替换导致无效代码时，不产生错误而是继续尝试其他候选函数
// - 常用于根据类型特性选择不同的函数实现，是C++中实现"编译期多态"的重要机制

// 以下示例展示如何使用SFINAE检测类型是否支持特定操作
template <typename T>
typename std::enable_if<std::is_integral<T>::value, bool>::type is_positive(T value)
{
  return value > 0;
}

// 启用仅当T是浮点类型时的函数
template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type is_positive(T value)
{
  return value > 0.0;
}

int main()
{
  // 1. 测试函数模板
  std::cout << "Function template examples:" << std::endl;
  std::cout << "max_value(10, 20): " << max_value(10, 20) << std::endl;
  std::cout << "max_value(3.14, 2.71): " << max_value(3.14, 2.71) << std::endl;
  std::cout << "max_value(\"apple\", \"banana\"): " << max_value<std::string>("apple", "banana") << std::endl;

  // 2. 测试类模板
  std::cout << "\nClass template examples:" << std::endl;
  Container<int> int_container(42);
  std::cout << "int_container.get_data(): " << int_container.get_data() << std::endl;

  Container<double> double_container(3.14159);
  std::cout << "double_container.get_data(): " << double_container.get_data() << std::endl;

  // 3. 测试模板特化
  std::cout << "\nTemplate specialization examples:" << std::endl;
  Container<std::string> string_container("Hello Templates!");
  std::cout << "string_container.get_data(): " << string_container.get_data() << std::endl;
  std::cout << "string_container.length(): " << string_container.length() << std::endl;

  // 4. 测试变参模板
  std::cout << "\nVariadic template examples:" << std::endl;
  std::cout << "sum(1): " << sum(1) << std::endl;
  std::cout << "sum(1, 2, 3, 4, 5): " << sum(1, 2, 3, 4, 5) << std::endl;
  std::cout << "sum(1.1, 2.2, 3.3): " << sum(1.1, 2.2, 3.3) << std::endl;

  // 5. 测试SFINAE
  std::cout << "\nSFINAE examples:" << std::endl;
  // - 对于整数类型，调用的是以">0"判断正负的版本
  std::cout << "is_positive(42): " << (is_positive(42) ? "true" : "false") << std::endl;
  std::cout << "is_positive(-42): " << (is_positive(-42) ? "true" : "false") << std::endl;
  // - 对于浮点类型，调用的是用">0.0"判断正负的版本
  std::cout << "is_positive(3.14): " << (is_positive(3.14) ? "true" : "false") << std::endl;
  std::cout << "is_positive(-3.14): " << (is_positive(-3.14) ? "true" : "false") << std::endl;

  std::cout << "\npassed!" << std::endl;
  return 0;
}