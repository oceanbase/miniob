#pragma once
#include "common/sys/rc.h"

// 只保留前置声明，不包含任何完整头
//class SQLStageEvent;

class DropTableExecutor {
public:
  RC execute(SQLStageEvent *sql_event);
};