/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Meiyi & Longda & Wangyunlai on 2021/5/12.
//

#pragma once

#include "common/sys/rc.h"
#include "common/lang/vector.h"
#include "common/lang/string.h"
#include "common/lang/unordered_map.h"
#include "common/lang/memory.h"
#include "common/lang/span.h"
#include "sql/parser/parse_defs.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/clog/disk_log_handler.h"
#include "storage/buffer/double_write_buffer.h"
#include "oblsm/include/ob_lsm.h"

class Table;
class LogHandler;
class BufferPoolManager;
class TrxKit;

/**
 * @brief 一个DB实例负责管理一批表
 * @details 当前DB的存储模式很简单，一个DB对应一个目录，所有的表和数据都放置在这个目录下。
 * 启动时，从指定的目录下加载所有的表和元数据。
 * 一个DB实例会有一个BufferPoolManager，用来管理所有的数据页，以及一个LogHandler，用来管理所有的日志。
 * 这样也就约束了事务不能跨DB。buffer pool的内存管理控制也不能跨越Db。
 * 也可以使用MiniOB非常容易模拟分布式事务，创建两个数据库，然后写一个分布式事务管理器。
 *
 * NOTE: 数据库对象没有做完整的并发控制。比如在查找某张表的同时删除这个表，会引起访问冲突。这个控制是由使用者
 * 来控制的。如果要完整的实现并发控制，需要实现表锁或类似的机制。
 */
class Db
{
public:
  Db() = default;
  ~Db();

  /**
   * @brief 初始化一个数据库实例
   * @details 从指定的目录下加载指定名称的数据库。这里就会加载dbpath目录下的数据。
   * @param name   数据库名称
   * @param dbpath 当前数据库放在哪个目录下
   * @param trx_kit_name 使用哪种类型的事务模型
   * @param storage_engine 存储引擎，目前只支持heap table 和 lsm-tree 两种
   * @note 数据库不是放在dbpath/name下，是直接使用dbpath目录
   * @todo 支持多个 db，例如同一个db 都是相同的存储引擎。可参考 duckdb。
   */
  RC init(const char *name, const char *dbpath, const char *trx_kit_name, const char *log_handler_name,
      const char *storage_engine = "heap");

  /**
   * @brief 创建一个表
   * @param table_name 表名
   * @param attributes 表的属性
   * @param storage_format 表的存储格式
   */
  RC create_table(const char *table_name, span<const AttrInfoSqlNode> attributes, const vector<string> &primary_keys,
      const StorageFormat storage_format = StorageFormat::ROW_FORMAT);

  /**
   * @brief 删除一个表
   * @param table_name 表名
   */
  RC drop_table(const char *table_name);

  /**
   * @brief 根据表名查找表
   */
  Table *find_table(const char *table_name) const;
  /**
   * @brief 根据表ID查找表
   */
  Table *find_table(int32_t table_id) const;

  /// @brief 当前数据库的名称
  const char *name() const;

  /// @brief 列出所有的表
  void all_tables(vector<string> &table_names) const;

  /**
   * @brief 将所有内存中的数据，刷新到磁盘中。
   * @details 注意，这里也没有并发控制，需要由上层来保证当前没有正在进行的事务。
   */
  RC sync();

  /// @brief 获取当前数据库的日志处理器
  LogHandler &log_handler();

  /// @brief 获取当前数据库的buffer pool管理器
  BufferPoolManager &buffer_pool_manager();

  /// @brief 获取当前数据库的事务管理器
  TrxKit &trx_kit();

  string path() const { return path_; }

  oceanbase::ObLsm *lsm() { return lsm_; }

private:
  /// @brief 打开所有的表。在数据库初始化的时候会执行
  RC open_all_tables();
  /// @brief 恢复数据。在数据库初始化的时候运行。
  RC recover();

  /// @brief 初始化元数据。在数据库初始化的时候，加载元数据
  RC init_meta();
  /// @brief 刷新数据库的元数据到磁盘中。每次执行sync时会执行此操作
  RC flush_meta();

  /// @brief 初始化数据库的double buffer pool
  RC init_dblwr_buffer();

  StorageEngine get_storage_engine()
  {
    StorageEngine engine = StorageEngine::UNKNOWN_ENGINE;
    if (storage_engine_.length() == 0) {
      engine = StorageEngine::HEAP;
    } else if (0 == strcasecmp(storage_engine_.c_str(), "heap")) {
      engine = StorageEngine::HEAP;
    } else if (0 == strcasecmp(storage_engine_.c_str(), "lsm")) {
      engine = StorageEngine::LSM;
    } else {
      engine = StorageEngine::UNKNOWN_ENGINE;
    }
    return engine;
  }

private:
  string                         name_;                 ///< 数据库名称
  string                         path_;                 ///< 数据库文件存放的目录
  unordered_map<string, Table *> opened_tables_;        ///< 当前所有打开的表
  unique_ptr<BufferPoolManager>  buffer_pool_manager_;  ///< 当前数据库的buffer pool管理器
  unique_ptr<LogHandler>         log_handler_;          ///< 当前数据库的日志处理器
  unique_ptr<TrxKit>             trx_kit_;              ///< 当前数据库的事务管理器
  oceanbase::ObLsm              *lsm_;                  ///< 当前数据库的 LSM-Tree 存储引擎

  /// 给每个table都分配一个ID，用来记录日志。这里假设所有的DDL都不会并发操作，所以相关的数据都不上锁
  int32_t next_table_id_ = 0;

  LSN    check_point_lsn_ = 0;  ///< 当前数据库的检查点LSN。会记录到磁盘中。
  string storage_engine_;
};
