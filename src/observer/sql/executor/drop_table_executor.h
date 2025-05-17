#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;

/**
 * @brief 删除表的执行器
 * 负责执行 DropTableStmt 指定的表删除操作
 */
class DropTableExecutor {
public:
  DropTableExecutor() = default;
  virtual ~DropTableExecutor() = default;

  RC execute(SQLStageEvent *sql_event);  // 执行函数声明
};
