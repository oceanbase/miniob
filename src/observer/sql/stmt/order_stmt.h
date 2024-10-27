#pragma once

#include "sql/expr/expression.h"
#include "sql/parser/parse_defs.h"
#include "sql/stmt/stmt.h"
#include "sql/stmt/order_stmt.h"
#include <vector>
#include <unordered_map>
#include "sql/parser/expression_binder.h"


class OrderUnit
{
public:
  OrderUnit() = default;
  ~OrderUnit() {}
  
  void setIncOrder(bool inc_order){
    this->inc_order_ = inc_order;
  }

  void setfield(Expression* bound_field_expr){
    this->bound_field_expr  = bound_field_expr;
  }

  bool get_inc_order(){ return inc_order_; }

  Expression* get_bound_field_expr(){ return bound_field_expr; }

private:
  bool inc_order_;   // 是否为升序
  Expression* bound_field_expr;      // 待排序字段
  
};

/**
 * @brief Order排序语句
 * @ingroup Statement
 */
class OrderStmt
{
public:
  OrderStmt(){};
  ~OrderStmt(){}

public:
  const std::vector<OrderUnit*> &order_units() const { return order_units_; }

public:
  static RC create(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables, const OrderSqlNode *orders, int ordernum, OrderStmt *&stmt, ExpressionBinder& expression_binder);

  static RC create_order_unit(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables, const OrderSqlNode& orderSqlNode, OrderUnit*& order_unit, ExpressionBinder& expression_binder);

private:
  std::vector<OrderUnit*> order_units_; 
};
