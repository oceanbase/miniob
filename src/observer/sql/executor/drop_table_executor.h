// zzw

#pragma once

#include "common/rc.h"  // 这个文件定义函数返回码/错误码(Return Code)

class SQLStageEvent;

/**
 * @brief 删除表的执行器
 * @ingroup Executor
 */
class DropTableExecutor
{
public:
  DropTableExecutor()          = default;
  virtual ~DropTableExecutor() = default;

  // 执行删除表的操作
  RC execute(SQLStageEvent *sql_event);
};