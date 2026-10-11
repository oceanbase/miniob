/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Sayan Pal on 2026/10/12.
//

#include <memory>

#include "sql/expr/expression.h"
#include "sql/optimizer/conjunction_simplification_rule.h"
#include "gtest/gtest.h"

using namespace std;
using namespace common;

// The rule does not fold comparisons, so this is not a constant to it, but it still evaluates to `value`
static unique_ptr<Expression> non_constant_expr(bool value)
{
  CompOp comp = value ? CompOp::EQUAL_TO : CompOp::NOT_EQUAL;
  return make_unique<ComparisonExpr>(comp, make_unique<ValueExpr>(Value(1)), make_unique<ValueExpr>(Value(1)));
}

static unique_ptr<Expression> bool_constant_expr(bool value) { return make_unique<ValueExpr>(Value(value)); }

template <typename... Children>
static unique_ptr<Expression> make_conjunction(ConjunctionExpr::Type type, Children... children)
{
  vector<unique_ptr<Expression>> child_exprs;
  (child_exprs.push_back(std::move(children)), ...);
  return make_unique<ConjunctionExpr>(type, child_exprs);
}

static bool simplify(unique_ptr<Expression> &expr)
{
  ConjunctionSimplificationRule rule;
  bool                          change_made = false;
  EXPECT_EQ(RC::SUCCESS, rule.rewrite(expr, change_made));
  return change_made;
}

static void expect_bool_constant(const unique_ptr<Expression> &expr, bool expected)
{
  ASSERT_EQ(ExprType::VALUE, expr->type());
  EXPECT_EQ(expected, static_cast<ValueExpr *>(expr.get())->get_value().get_boolean());
}

TEST(ConjunctionSimplificationRule, and_with_false_is_false)
{
  // (1 = 1) AND false => false
  unique_ptr<Expression> expr =
      make_conjunction(ConjunctionExpr::Type::AND, non_constant_expr(true), bool_constant_expr(false));
  EXPECT_TRUE(simplify(expr));
  expect_bool_constant(expr, false);
}

TEST(ConjunctionSimplificationRule, or_with_true_is_true)
{
  // (1 <> 1) OR true => true
  unique_ptr<Expression> expr =
      make_conjunction(ConjunctionExpr::Type::OR, non_constant_expr(false), bool_constant_expr(true));
  EXPECT_TRUE(simplify(expr));
  expect_bool_constant(expr, true);
}

TEST(ConjunctionSimplificationRule, all_children_removed)
{
  // true AND true => true
  unique_ptr<Expression> and_expr =
      make_conjunction(ConjunctionExpr::Type::AND, bool_constant_expr(true), bool_constant_expr(true));
  EXPECT_TRUE(simplify(and_expr));
  expect_bool_constant(and_expr, true);

  // false OR false => false
  unique_ptr<Expression> or_expr =
      make_conjunction(ConjunctionExpr::Type::OR, bool_constant_expr(false), bool_constant_expr(false));
  EXPECT_TRUE(simplify(or_expr));
  expect_bool_constant(or_expr, false);
}

TEST(ConjunctionSimplificationRule, identity_constant_is_removed)
{
  // (1 = 1) AND true AND (1 <> 1) => (1 = 1) AND (1 <> 1)
  unique_ptr<Expression> left      = non_constant_expr(true);
  unique_ptr<Expression> right     = non_constant_expr(false);
  Expression            *left_ptr  = left.get();
  Expression            *right_ptr = right.get();
  unique_ptr<Expression> expr =
      make_conjunction(ConjunctionExpr::Type::AND, std::move(left), bool_constant_expr(true), std::move(right));
  EXPECT_TRUE(simplify(expr));
  ASSERT_EQ(ExprType::CONJUNCTION, expr->type());
  vector<unique_ptr<Expression>> *children = &static_cast<ConjunctionExpr *>(expr.get())->children();
  ASSERT_EQ(2, children->size());
  EXPECT_EQ(left_ptr, (*children)[0].get());
  EXPECT_EQ(right_ptr, (*children)[1].get());

  // (1 = 1) OR false OR (1 <> 1) => (1 = 1) OR (1 <> 1)
  left      = non_constant_expr(true);
  right     = non_constant_expr(false);
  left_ptr  = left.get();
  right_ptr = right.get();
  expr      = make_conjunction(ConjunctionExpr::Type::OR, std::move(left), bool_constant_expr(false), std::move(right));
  EXPECT_TRUE(simplify(expr));
  ASSERT_EQ(ExprType::CONJUNCTION, expr->type());
  children = &static_cast<ConjunctionExpr *>(expr.get())->children();
  ASSERT_EQ(2, children->size());
  EXPECT_EQ(left_ptr, (*children)[0].get());
  EXPECT_EQ(right_ptr, (*children)[1].get());
}

TEST(ConjunctionSimplificationRule, single_child_left_replaces_conjunction)
{
  // (1 <> 1) AND true => (1 <> 1)
  unique_ptr<Expression> child     = non_constant_expr(false);
  Expression            *child_ptr = child.get();
  unique_ptr<Expression> expr =
      make_conjunction(ConjunctionExpr::Type::AND, std::move(child), bool_constant_expr(true));
  EXPECT_TRUE(simplify(expr));
  EXPECT_EQ(child_ptr, expr.get());
}

TEST(ConjunctionSimplificationRule, no_constant_no_change)
{
  // (1 = 1) AND (1 <> 1) has nothing to simplify
  unique_ptr<Expression> expr =
      make_conjunction(ConjunctionExpr::Type::AND, non_constant_expr(true), non_constant_expr(false));
  Expression *origin = expr.get();
  EXPECT_FALSE(simplify(expr));
  EXPECT_EQ(origin, expr.get());
  EXPECT_EQ(2, static_cast<ConjunctionExpr *>(expr.get())->children().size());
}

int main(int argc, char **argv)
{

  // 分析gtest程序的命令行参数
  testing::InitGoogleTest(&argc, argv);

  // 调用RUN_ALL_TESTS()运行所有测试用例
  // main函数返回RUN_ALL_TESTS()的运行结果
  return RUN_ALL_TESTS();
}
