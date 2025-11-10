#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;

/**
 * 执行 DROP TABLE 的 executor
 */
class DropTableExecutor
{
public:
  static RC execute(SQLStageEvent *sql_event);
};
