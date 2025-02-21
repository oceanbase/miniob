/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// lambda 表达式允许我们在另一个函数内定义匿名函数。
// 嵌套很重要，因为它使我们既可以避免名称空间命名污染，
// 又可以将函数定义为尽可能靠近其使用位置（提供额外的上下文）。
// Lambda 的形式如下：
// [ captureClause ] ( parameters ) -> returnType
// {
//     statements;
// }
// 如果不需要 `capture`，则 `captureClause` 可以为空。
// 如果不需要 `parameters`，`parameters` 可以为空。除非指定返回类型，否则也可以完全省略它。
// `returnType` 是可选的，如果省略，则将假定为 auto（使用类型推导来确定返回类型）。

#include <iostream>
#include <vector>
#include <cassert>

int main()
{
  std::vector<int> numbers = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};

  // TODO: 定义一个 lambda 表达式来检查数字是否为偶数

  // 使用 lambda 表达式来累计所有偶数的和
  int even_sum = 0;
  for (const auto &num : numbers) {
    (void)(num);
    // TODO: 在实现 lambda 表达式后将下面的注释取消注释
    // if (is_even(num)) {
    //     even_sum += num;
    // }
  }

  std::cout << "Sum of even numbers: " << even_sum << std::endl;
  assert(even_sum == 12);
  std::cout << "passed!" << std::endl;
  return 0;
}
