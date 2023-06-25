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
// Created by Wangyunlai on 2023/06/16.
//

#pragma once

#include "net/ring_buffer.h"

/**
 * @brief 支持以缓存模式写入数据到文件/socket
 * @details 缓存使用ring buffer实现，当缓存满时会自动刷新缓存。
 * 看起来直接使用fdopen也可以实现缓存写，不过fdopen会在close时直接关闭fd。
 * @note 在执行close时，描述符fd并不会被关闭
 */
class BufferedWriter
{
public:
  BufferedWriter(int fd);
  BufferedWriter(int fd, int32_t size);
  ~BufferedWriter();

  /**
   * @brief 关闭缓存
   */
  RC close();

  /**
   * @brief 写数据到文件/socket
   * @details 缓存满会自动刷新缓存
   * @param data 要写入的数据
   * @param size 要写入的数据大小 
   * @param write_size 实际写入的数据大小
   */
  RC write(const char *data, int32_t size, int32_t &write_size);

  /**
   * @brief 写数据到文件/socket，全部写入成功返回成功
   * @details 与write的区别就是会尝试一直写直到写成成功或者有不可恢复的错误
   * @param data 要写入的数据
   * @param size 要写入的数据大小
   */
  RC writen(const char *data, int32_t size);

  /**
   * @brief 刷新缓存
   * @details 将缓存中的数据全部写入文件/socket
   */
  RC flush();

private:
  /**
   * @brief 刷新缓存
   * @details 期望缓存可以刷新size大小的数据，实际刷新的数据量可能小于size也可能大于size。
   * 通常是在缓存满的时候，希望刷新掉一部分数据，然后继续写入。
   * @param size 期望刷新的数据大小
   */
  RC flush_internal(int32_t size);

private:
  int fd_ = -1;
  RingBuffer buffer_;
};