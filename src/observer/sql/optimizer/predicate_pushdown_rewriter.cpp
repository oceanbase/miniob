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
// Created by Wangyunlai on 2022/12/30.
//

#include "sql/optimizer/predicate_pushdown_rewriter.h"
#include "sql/operator/logical_operator.h"
#include "sql/operator/table_get_logical_operator.h"
#include "sql/operator/join_logical_operator.h"
#include "sql/operator/predicate_logical_operator.h"
#include "sql/expr/expression.h"

RC PredicatePushdownRewriter::rewrite(std::unique_ptr<LogicalOperator> &oper, bool &change_made)
{
  RC rc = RC::SUCCESS;
  if (oper->type() != LogicalOperatorType::PREDICATE) {
    return rc;
  }

  if (oper->children().size() != 1) {
    return rc;
  }

  std::unique_ptr<LogicalOperator> &child_oper = oper->children().front();
  if (!(child_oper->type() == LogicalOperatorType::TABLE_GET || child_oper->type() == LogicalOperatorType::JOIN)) {
    return rc;
  }

  // auto table_get_oper = static_cast<TableGetLogicalOperator *>(child_oper.get());

  std::vector<std::unique_ptr<Expression>> &predicate_oper_exprs = oper->expressions();
  if (predicate_oper_exprs.size() != 1) {
    return rc;
  }

  std::unique_ptr<Expression> &predicate_expr = predicate_oper_exprs.front();
  std::vector<std::unique_ptr<Expression>> pushdown_exprs;
  rc = get_exprs_can_pushdown(predicate_expr, pushdown_exprs);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get exprs can pushdown. rc=%s", strrc(rc));
    return rc;
  }

  if (pushdown_exprs.empty())
    return rc;

  change_made = false;
  //pushdown
  if (child_oper->type() == LogicalOperatorType::TABLE_GET) {
    auto table_get_oper = static_cast<TableGetLogicalOperator *>(child_oper.get());
    change_made = true;
    table_get_oper->set_predicates(std::move(pushdown_exprs));
  }
  else {
    auto join_oper = static_cast<JoinLogicalOperator *>(child_oper.get());
    auto left_join_child_type = join_oper->children()[0]->type();
    auto right_join_child_type = join_oper->children()[1]->type();

    if(left_join_child_type == LogicalOperatorType::JOIN && right_join_child_type == LogicalOperatorType::TABLE_GET) {
      auto right_table_get_oper = static_cast<TableGetLogicalOperator*>(join_oper->children()[1].get());
      const char* right_table_name = right_table_get_oper->table()->name();
      std::vector<std::unique_ptr<Expression>> right_child_exprs;

      for(auto it = pushdown_exprs.begin(); it != pushdown_exprs.end(); ) {
        auto comparison_expr = static_cast<ComparisonExpr *>((*it).get());

        bool related_to_right_table = false;
        if (comparison_expr->left()->type() == ExprType::FIELD) {
          auto left_expr = static_cast<FieldExpr*>(comparison_expr->left().get());
          related_to_right_table = related_to_right_table || (strcmp(left_expr->table_name(), right_table_name) == 0);
        }
        if (comparison_expr->right()->type() == ExprType::FIELD) {
          auto right_expr = static_cast<FieldExpr*>(comparison_expr->right().get());
          related_to_right_table = related_to_right_table || (strcmp(right_expr->table_name(), right_table_name) == 0);
        }

        if(related_to_right_table) {
          right_child_exprs.push_back(std::move(*it));
        }

        if(!*it) {
          pushdown_exprs.erase(it);
        } else {
          it++;
        } 
      }

      predicate_expr = std::unique_ptr<Expression>(new ConjunctionExpr(ConjunctionExpr::Type::AND, right_child_exprs));
      std::unique_ptr<ConjunctionExpr>conjunction_expr(new ConjunctionExpr(ConjunctionExpr::Type::AND, pushdown_exprs));
      std::unique_ptr<PredicateLogicalOperator>left_new_oper = std::unique_ptr<PredicateLogicalOperator>(new PredicateLogicalOperator(std::move(conjunction_expr)));
      left_new_oper->add_child(std::move(child_oper->children()[0]));
      auto temp_oper = std::move(child_oper->children()[1]);
      child_oper->children().clear();
      child_oper->add_child(std::move(left_new_oper));
      child_oper->add_child(std::move(temp_oper));
    }
    else {
      if (pushdown_exprs.size() == 1)
        predicate_expr = std::move(pushdown_exprs.front());
      else 
        predicate_expr = std::unique_ptr<Expression>(new ConjunctionExpr(ConjunctionExpr::Type::AND, pushdown_exprs));
    }
  }

  if (!predicate_expr) {
    // 所有的表达式都下推到了下层算子
    // 这个predicate operator其实就可以不要了。但是这里没办法删除，弄一个空的表达式吧
    LOG_TRACE("all expressions of predicate operator were pushdown to table get operator, then make a fake one");

    Value value((bool)true);
    predicate_expr = std::unique_ptr<Expression>(new ValueExpr(value));
  }

  // if (!pushdown_exprs.empty()) {
  //   change_made = true;
  //   table_get_oper->set_predicates(std::move(pushdown_exprs));
  // }
  return rc;
}

/**
 * 查看表达式是否可以直接下放到table get算子的filter
 * @param expr 是当前的表达式。如果可以下放给table get 算子，执行完成后expr就失效了
 * @param pushdown_exprs 当前所有要下放给table get 算子的filter。此函数执行多次，
 *                       pushdown_exprs 只会增加，不要做清理操作
 */
RC PredicatePushdownRewriter::get_exprs_can_pushdown(
    std::unique_ptr<Expression> &expr, std::vector<std::unique_ptr<Expression>> &pushdown_exprs)
{
  RC rc = RC::SUCCESS;
  if (expr->type() == ExprType::CONJUNCTION) {
    ConjunctionExpr *conjunction_expr = static_cast<ConjunctionExpr *>(expr.get());
    // 或 操作的比较，太复杂，现在不考虑
    if (conjunction_expr->conjunction_type() == ConjunctionExpr::Type::OR) {
      return rc;
    }

    std::vector<std::unique_ptr<Expression>> &child_exprs = conjunction_expr->children();
    for (auto iter = child_exprs.begin(); iter != child_exprs.end();) {
      // 对每个子表达式，判断是否可以下放到table get 算子
      // 如果可以的话，就从当前孩子节点中删除他
      rc = get_exprs_can_pushdown(*iter, pushdown_exprs);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to get pushdown expressions. rc=%s", strrc(rc));
        return rc;
      }

      if (!*iter) {
        child_exprs.erase(iter);
      } else {
        ++iter;
      }
    }
  } else if (expr->type() == ExprType::COMPARISON) {
    // 如果是比较操作，并且比较的左边或右边是表某个列值，那么就下推下去
    auto comparison_expr = static_cast<ComparisonExpr *>(expr.get());
    CompOp comp = comparison_expr->comp();
    // if (comp != EQUAL_TO) {
    //   // 简单处理，仅取等值比较。当然还可以取一些范围比较，还有 like % 等操作
    //   // 其它的还有 is null 等
    //   return rc;
    // }
    if (comp >= NO_OP) {
      // 简单处理，仅取等值比较。当然还可以取一些范围比较，还有 like % 等操作
      // 其它的还有 is null 等
      return rc;
    }

    std::unique_ptr<Expression> &left_expr = comparison_expr->left();
    std::unique_ptr<Expression> &right_expr = comparison_expr->right();
    // 比较操作的左右两边只要有一个是取列字段值的并且另一边也是取字段值或常量，就pushdown
    if (left_expr->type() != ExprType::FIELD && right_expr->type() != ExprType::FIELD) {
      return rc;
    }
    if (left_expr->type() != ExprType::FIELD && left_expr->type() != ExprType::VALUE &&
        right_expr->type() != ExprType::FIELD && right_expr->type() != ExprType::VALUE) {
      return rc;
    }

    pushdown_exprs.emplace_back(std::move(expr));
  }
  return rc;
}
