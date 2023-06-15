/* Copyright (c) 2021-2022 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by huhaosheng.hhs on 2022
//

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <list>
#include <atomic>
#include <unordered_map>
#include <deque>
#include <memory>
#include <string>

#include "storage/record/record.h"
#include "storage/persist/persist.h"
#include "common/lang/mutex.h"

class CLogManager;
class CLogBuffer;
class CLogFile;
class Db;

/**
 * @defgroup CLog
 * @brief CLog 就是 commit log。或者等价于redo log。
 * @file clog.h
 * @details 这个模块想要实现数据库事务中的D(durability)，也就是持久化。
 * 持久化是事务四大特性(ACID)中最复杂的模块，这里的实现简化了99.999%，仅在一些特定场景下才能
 * 恢复数据库。
 */

/**
 * @enum CLogType
 * @ingroup CLog
 * @brief 定义clog的几种类型
 * @details 除了事务操作相关的类型，比如MTR_BEGIN/MTR_COMMIT等，都是需要事务自己去处理的。
 * 也就是说，像INSERT、DELETE等是事务自己处理的，其实这种类型的日志不需要在这里定义，而是在各个
 * 事务模型中定义，由各个事务模型自行处理。
 */
#define DEFINE_CLOG_TYPE_ENUM         \
  DEFINE_CLOG_TYPE(ERROR)             \
  DEFINE_CLOG_TYPE(MTR_BEGIN)         \
  DEFINE_CLOG_TYPE(MTR_COMMIT)        \
  DEFINE_CLOG_TYPE(MTR_ROLLBACK)      \
  DEFINE_CLOG_TYPE(INSERT)            \
  DEFINE_CLOG_TYPE(DELETE)

enum class CLogType 
{ 
#define DEFINE_CLOG_TYPE(name) name,
  DEFINE_CLOG_TYPE_ENUM
#undef DEFINE_CLOG_TYPE
};

/**
 * @brief clog type 转换成字符串
 * @ingroup CLog
 */
const char *clog_type_name(CLogType type);

/**
 * @brief clog type 转换成数字
 */
int32_t clog_type_to_integer(CLogType type);

/**
 * @brief 数字转换成clog type
 * @ingroup CLog
 */
CLogType clog_type_from_integer(int32_t value);

/**
 * @brief CLog的记录头。每个日志都带有这个信息
 * @ingroup CLog
 */
struct CLogRecordHeader 
{
  int32_t lsn_ = -1;     ///< log sequence number。当前没有使用
  int32_t trx_id_ = -1;  ///< 日志所属事务的编号
  int32_t type_ = clog_type_to_integer(CLogType::ERROR); ///< 日志类型
  int32_t logrec_len_ = 0;  ///< record的长度，不包含header长度

  bool operator==(const CLogRecordHeader &other) const
  {
    return lsn_ == other.lsn_ && trx_id_ == other.trx_id_ && type_ == other.type_ && logrec_len_ == other.logrec_len_;
  }

  std::string to_string() const;
};

/**
 * @ingroup CLog
 * @brief MTR_COMMIT 日志的数据
 * @details 其它的类型的MTR日志都没有数据，只有COMMIT有。
 */
struct CLogRecordCommitData
{
  int32_t commit_xid_ = -1; ///< 事务提交的事务号

  bool operator == (const CLogRecordCommitData &other) const
  {
    return this->commit_xid_ == other.commit_xid_;
  }

  std::string to_string() const;
};

/**
 * @brief 有具体数据修改的事务日志数据
 * @ingroup CLog
 * @details 这里记录的都是操作的记录，比如插入、删除一条数据。
 */
struct CLogRecordData
{
  int32_t          table_id_ = -1;    ///< 操作的表
  RID              rid_;              ///< 操作的哪条记录
  int32_t          data_len_ = 0;     ///< 记录的数据长度(因为header中也包含长度信息，这个长度可以不要)
  int32_t          data_offset_ = 0;  ///< 操作的数据在完整记录中的偏移量
  char *           data_ = nullptr;   ///< 具体的数据，可能没有任何数据

  ~CLogRecordData();

  bool operator==(const CLogRecordData &other) const
  {
    return table_id_ == other.table_id_ &&
      rid_ == other.rid_ &&
      data_len_ == other.data_len_ &&
      data_offset_ == other.data_offset_ &&
      0 == memcmp(data_, other.data_, data_len_);
  }

  std::string to_string() const;

  const static int32_t HEADER_SIZE;  ///< 指RecordData的头长度，即不包含data_的长度
};

/**
 * @brief 表示一条日志记录
 * @ingroup CLog
 * @details 一条日志记录由一个日志头和具体的数据构成。
 * 具体的数据根据日志类型不同，也是不同的类型。
 */
class CLogRecord 
{
public:
  /**
   * @brief 默认构造函数。
   * @details 通常不需要直接调用这个函数来创建一条日志，而是调用 `build_xxx`创建对象。
   */
  CLogRecord() = default;

  ~CLogRecord();

  /**
   * @brief 创建一个事务相关的日志对象
   * @details 除了MTR_COMMIT的日志(请参考 build_commit_record)。
   * @param type 日志类型
   * @param trx_id 事务编号
   */
  static CLogRecord *build_mtr_record(CLogType type, int32_t trx_id);

  /**
   * @brief 创建一个表示提交事务的日志对象
   * 
   * @param trx_id 事务编号
   * @param commit_xid 事务提交时使用的编号
   */
  static CLogRecord *build_commit_record(int32_t trx_id, int32_t commit_xid);

  /**
   * @brief 创建一个表示数据操作的日志对象
   * 
   * @param type 类型
   * @param trx_id 事务编号
   * @param table_id 操作的表
   * @param rid 操作的哪条记录
   * @param data_len 数据的长度
   * @param data_offset 偏移量，参考 CLogRecordData::data_offset_
   * @param data 具体的数据
   */
  static CLogRecord *build_data_record(CLogType type,
                                       int32_t trx_id,
                                       int32_t table_id,
                                       const RID &rid,
                                       int32_t data_len,
                                       int32_t data_offset,
                                       const char *data);
  
  /**
   * @brief 根据二进制数据创建日志对象
   * @details 通常是从日志文件中读取数据，然后调用此函数创建日志对象
   * @param header 日志头信息
   * @param data   读取的剩余数据信息，长度是header.logrec_len_
   */
  static CLogRecord *build(const CLogRecordHeader &header, char *data);

  CLogType log_type() const  { return clog_type_from_integer(header_.type_); }
  int32_t  trx_id() const { return header_.trx_id_; }
  int32_t  logrec_len() const { return header_.logrec_len_; }

  CLogRecordHeader &header() { return header_; }
  CLogRecordCommitData &commit_record() { return commit_record_; }
  CLogRecordData   &data_record() { return data_record_; }

  const CLogRecordHeader &header() const { return header_; }
  const CLogRecordCommitData &commit_record() const { return commit_record_; }
  const CLogRecordData   &data_record() const { return data_record_; }

  std::string to_string() const;

protected:
  CLogRecordHeader header_; ///< 日志头信息

  CLogRecordData       data_record_;   ///< 如果日志操作的是数据，此结构生效
  CLogRecordCommitData commit_record_; ///< 如果是事务提交日志，此结构生效
};

/**
 * @brief 缓存运行时产生的日志对象
 * @ingroup CLog
 * @details 当前的实现非常简单，没有采用其它数据库中常用的将日志序列化到二进制buffer，
 * 管理二进制buffer的方法。这里仅仅把日志记录下来，放到链表中。如果达到一定量的日志，
 * 或者日志数量超过某个阈值，就会调用flush_buffer将日志刷新到磁盘中。
 */
class CLogBuffer 
{
public:
  CLogBuffer();
  ~CLogBuffer();

  /**
   * @brief 增加一条日志
   * @details 如果当前的日志达到一定量，就会刷新数据
   */
  RC append_log_record(CLogRecord *log_record);

  /**
   * @brief 将当前的日志都刷新到日志文件中
   * @details 因为多线程访问与日志管理的问题，只能有一个线程调用此函数
   * @param log_file 日志文件
   */
  RC flush_buffer(CLogFile &log_file);

private:
  /**
   * @brief 将日志记录写入到日志文件中
   * 
   * @param log_file 日志文件，概念上来讲不一定是某个特定的文件
   * @param log_record 要写入的日志记录
   */
  RC write_log_record(CLogFile &log_file, CLogRecord *log_record);

private:
  common::Mutex lock_;  ///< 加锁支持多线程并发写入
  std::deque<std::unique_ptr<CLogRecord>> log_records_;  ///< 当前等待刷数据的日志记录
  std::atomic_int32_t total_size_;  ///< 当前缓存中的日志记录的总大小
};

/**
 * @brief 读写日志文件
 * @ingroup CLog
 * @details 这里的名字不太贴切，因为这个类希望管理所有日志文件，而不是特定的某个文件。不过当前
 * 只有一个文件，并且文件名是固定的。
 */
class CLogFile 
{
public:
  CLogFile() = default;
  ~CLogFile();

  /**
   * @brief 初始化
   * 
   * @param path 日志文件存放的路径。会打开这个目录下叫做 clog 的文件。
   */
  RC init(const char *path);

  /**
   * @brief 写入指定数据，全部写入成功返回成功，否则返回失败
   * @details 作为日志文件读写的类，实现一个write_log_record可能更合适。
   * @note  如果日志文件写入一半失败了，应该做特殊处理，但是这里什么都没管。
   * @param data 写入的数据
   * @param len  数据的长度
   */
  RC write(const char *data, int len);

  /**
   * @brief 读取指定长度的数据。全部读取成功返回成功，否则返回失败
   * @details 与 write 有类似的问题。如果读取到了文件尾，会标记eof，可以通过eof()函数来判断。
   * @param data 数据读出来放这里
   * @param len  读取的长度
   */
  RC read(char *data, int len);

  /**
   * @brief 将当前写的文件执行sync同步数据到磁盘
   */
  RC sync();

  /**
   * @brief 获取当前读取的文件位置
   */
  RC offset(int64_t &off) const;

  /**
   * @brief 当前是否已经读取到文件尾
   */
  bool eof() const { return eof_; }

protected:
  std::string filename_;  ///< 日志文件名。总是init函数参数path路径下的clog文件
  int fd_ = -1;           ///< 操作的文件描述符
  bool eof_ = false;      ///< 是否已经读取到文件尾
};

/**
 * @brief 日志记录遍历器
 * @ingroup CLog
 * @details 使用时先执行初始化(init)，然后多次调用next，直到valid返回false。
 */
class CLogRecordIterator
{
public:
  CLogRecordIterator() = default;
  ~CLogRecordIterator() = default;

  RC init(CLogFile &log_file);

  bool valid() const;
  RC next();
  const CLogRecord &log_record();

private:
  CLogFile *log_file_ = nullptr;
  CLogRecord *log_record_ = nullptr;
};

/**
 * @brief 日志管理器
 * @ingroup CLog
 * @details 一个日志管理器属于某一个DB（当前仅有一个DB sys）。
 * 管理器负责写日志（运行时）、读日志与恢复（启动时）
 */
class CLogManager 
{
public:
  CLogManager() = default;
  ~CLogManager();

  /**
   * @brief 初始化日志管理器
   * 
   * @param path 日志都放在这个目录下。当前就是数据库的目录
   */
  RC init(const char *path);

  /**
   * @brief 新增一条数据更新的日志
   */
  RC append_log(CLogType type,
                int32_t trx_id,
                int32_t table_id,
                const RID &rid,
                int32_t data_len,
                int32_t data_offset,
                const char *data);

  /**
   * @brief 开启一个事务
   * 
   * @param trx_id 事务编号
   */
  RC begin_trx(int32_t trx_id);

  /**
   * @brief 提交一个事务
   * 
   * @param trx_id 事务编号
   * @param commit_xid 事务提交时使用的编号
   */
  RC commit_trx(int32_t trx_id, int32_t commit_xid);

  /**
   * @brief 回滚一个事务
   * 
   * @param trx_id 事务编号
   */
  RC rollback_trx(int32_t trx_id);

  /**
   * @brief 也可以调用这个函数直接增加一条日志
   */
  RC append_log(CLogRecord *log_record);

  /**
   * @brief 刷新日志到磁盘
   */
  RC sync();

  /**
   * @brief 重做
   * @details 当前会重做所有日志。也就是说，所有buffer pool页面都不会写入到磁盘中，
   * 否则可能无法恢复成功。
   */
  RC recover(Db *db);

private:
  CLogBuffer *log_buffer_ = nullptr;   ///< 日志缓存。新增日志时先放到内存，也就是这个buffer中
  CLogFile *  log_file_   = nullptr;   ///< 管理日志，比如读写日志
};
