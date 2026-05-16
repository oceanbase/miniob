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
// Created by wangyunlai on 2022/02/01
//

#include "storage/buffer/buffer_pool_log.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/clog/log_handler.h"
#include "storage/clog/log_entry.h"

string BufferPoolLogEntry::to_string() const
{
  return string("buffer_pool_id=") + std::to_string(buffer_pool_id) +
         ", page_num=" + std::to_string(page_num) +
         ", operation_type=" + BufferPoolOperation(operation_type).to_string();
}
/**
 * @brief 将BufferPoolLogEntry序列化为可读字符串
 * @details 用于日志输出和调试，展示buffer_pool_id、page_num和operation_type
 */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BufferPoolLogHandler::BufferPoolLogHandler(DiskBufferPool &buffer_pool, LogHandler &log_handler)
    : buffer_pool_(buffer_pool), log_handler_(log_handler)
{}
/**
 * @brief 构造函数
 * @param buffer_pool 关联的磁盘缓冲池，用于获取buffer_pool_id
 * @param log_handler 日值处理器，负责实际的日志追加和持久化
 */
RC BufferPoolLogHandler::allocate_page(PageNum page_num, LSN &lsn)
{
  return append_log(BufferPoolOperation::Type::ALLOCATE, page_num, lsn);
}
/**
 * @brief 记录页面分配日志
 * @param page_num 待分配的页号
 * @param[out] lsn 返回分配操作对应的日志序列号
 * @return RC 操作结果
 * @note 在内存中分配页面前调用，确保崩溃后可重做
 */
RC BufferPoolLogHandler::deallocate_page(PageNum page_num, LSN &lsn)
{
  return append_log(BufferPoolOperation::Type::DEALLOCATE, page_num, lsn);
}
/**
 * @brief 记录页面释放日志
 * @param page_num 待释放的页号
 * @param[out] lsn 返回释放操作对应的日志序列号
 * @return RC 操作结果
 * @note 在内存中释放页面前调用，确保崩溃后可重做
 */
RC BufferPoolLogHandler::flush_page(Page &page)
{
  return log_handler_.wait_lsn(page.lsn);
}
/**
 * @brief 等待页面关联的日志持久化到磁盘
 * @param page 需要确保日志已刷盘的页面
 * @return RC 操作结果
 * @details 在将脏页刷盘前调用，保证 WAL（Write-Ahead Logging）原则：
 *          必须先写日志，再写数据页。通过等待 page.lsn 之前的所有日志落盘，
 *          确保崩溃恢复时可以通过日志重做该页面的修改。
 */
RC BufferPoolLogHandler::append_log(BufferPoolOperation::Type type, PageNum page_num, LSN &lsn)
{
  BufferPoolLogEntry log;
  log.buffer_pool_id = buffer_pool_.id();
  log.page_num = page_num;
  log.operation_type = BufferPoolOperation(type).type_id();

  return log_handler_.append(lsn, LogModule::Id::BUFFER_POOL, span<const char>(reinterpret_cast<const char *>(&log), sizeof(log)));
}
/**
 * @brief 内部方法：构造并追加 Buffer Pool 日志条目
 * @param type 操作类型（ALLOCATE / DEALLOCATE）
 * @param page_num 目标页号
 * @param[out] lsn 返回日志序列号
 * @return RC 操作结果
 * @note 使用 span 传递二进制数据，避免额外的内存拷贝
 */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BufferPoolLogReplayer
BufferPoolLogReplayer::BufferPoolLogReplayer(BufferPoolManager &bp_manager) : bp_manager_(bp_manager)
{}
/**
 * @brief 构造函数
 * @param bp_manager Buffer Pool 管理器，用于根据 id 查找对应的 DiskBufferPool
 */
RC BufferPoolLogReplayer::replay(const LogEntry &entry)
{
  if (entry.payload_size() != sizeof(BufferPoolLogEntry)) {
    LOG_ERROR("invalid buffer pool log entry. payload size=%d, expected=%d, entry=%s",
              entry.payload_size(), sizeof(BufferPoolLogEntry), entry.to_string().c_str());
    return RC::INVALID_ARGUMENT;
  }//校验日志负载大小，防止数据损坏或者格式不匹配

  auto log = reinterpret_cast<const BufferPoolLogEntry *>(entry.data());
  //将二进制负载解析为BufferPoolLogEntry
  //注意：这里依赖了结构体内存布局的稳定性（POD类型）
  LOG_TRACE("replay buffer pool log. entry=%s, log=%s", entry.to_string().c_str(), log->to_string().c_str());
  
  int32_t buffer_pool_id = log->buffer_pool_id;
  //根据buffer_pool_id定位目标DiskBufferPool
  DiskBufferPool *buffer_pool = nullptr;
  RC rc = bp_manager_.get_buffer_pool(buffer_pool_id, buffer_pool);
  if (OB_FAIL(rc) || buffer_pool == nullptr) {
    LOG_ERROR("failed to get buffer pool. rc=%s, buffer pool=%p, log=%s, %s", 
              strrc(rc), buffer_pool, entry.to_string().c_str(), log->to_string().c_str());
    return rc;
  }

  BufferPoolOperation operation(log->operation_type);
  switch (operation.type())
  {
    case BufferPoolOperation::Type::ALLOCATE:
      return buffer_pool->redo_allocate_page(entry.lsn(), log->page_num);
    case BufferPoolOperation::Type::DEALLOCATE:
      return buffer_pool->redo_deallocate_page(entry.lsn(), log->page_num);
    default:
      LOG_ERROR("unknown buffer pool operation. operation=%s", operation.to_string().c_str());
      return RC::INTERNAL;
  }//根据操作类型分发到对应的redo方法
  return RC::SUCCESS;//不可达代码，但保留以消除编译器警告
}
/**
 * @brief 重放单条 Buffer Pool 日志
 * @param entry 从日志文件中读取的日志条目
 * @return RC 操作结果
 * @details 崩溃恢复时由日志回放框架调用，根据日志内容重做页面分配/释放操作。
 *          这是 REDO 日志的核心逻辑：重复执行操作是幂等的（幂等性由底层保证）。
 */