#include "sql/stmt/order_stmt.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "sql/parser/expression_binder.h"
#include "common/rc.h"

RC OrderStmt::create(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
                     const OrderSqlNode *orders, int ordernum, OrderStmt *&stmt, ExpressionBinder& expression_binder){
  
  RC rc = RC::SUCCESS;
  stmt  = nullptr;
  
  // 为每一个排序属性创建 OrderUnit 对象，并放入 OrderStmt 的 order_units_之中
  OrderStmt* order_stmt = new OrderStmt();
  for (int i = 0; i < ordernum; i++) {
    OrderUnit* order_unit = nullptr;
    // 创建 order_units_ 并为其中的 UnboundFieldExpr 绑定
    rc = create_order_unit(db, default_table, tables, orders[i], order_unit, expression_binder);
    if (rc != RC::SUCCESS) {
      delete order_stmt;
      LOG_WARN("failed to create order unit. order index=%d", i);
      return rc;
    }
    order_stmt->order_units_.push_back(order_unit);
  }

  stmt = order_stmt;
  return rc;
}

RC create_order_unit(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
                     const OrderSqlNode& orderSqlNode, OrderUnit*& order_unit, ExpressionBinder& expression_binder){

    std::unique_ptr<Expression> tmp = std::unique_ptr<Expression>(orderSqlNode.unbound_field_expr_);
    vector<unique_ptr<Expression>> bound_expressions;
    expression_binder.bind_expression(tmp, bound_expressions);

    order_unit = new OrderUnit();
    order_unit -> setfield(bound_expressions.at(0).release());
    order_unit -> setIncOrder(orderSqlNode.inc_order);
    return RC::SUCCESS;
}