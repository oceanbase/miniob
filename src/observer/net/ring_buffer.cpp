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

#include <algorithm>

#include "common/log/log.h"
#include "net/ring_buffer.h"

using namespace std;

const int32_t DEFAULT_BUFFER_SIZE = 16 * 1024;

RingBuffer::RingBuffer() : RingBuffer(DEFAULT_BUFFER_SIZE) {}

RingBuffer::RingBuffer(int32_t size) : buffer_(size) {}

RingBuffer::~RingBuffer() {}

RC RingBuffer::read(char *buf, int32_t size, int32_t &read_size)
{
  if (size < 0) {
    return RC::INVALID_ARGUMENT;
  }

  RC rc     = RC::SUCCESS;
  read_size = 0;
  while (OB_SUCC(rc) && read_size<size &&this->size()> 0) {
    const char *tmp_buf  = nullptr;
    int32_t     tmp_size = 0;
    rc                   = buffer(tmp_buf, tmp_size);
    if (OB_SUCC(rc)) {
      int32_t copy_size = min(size - read_size, tmp_size);
      memcpy(buf + read_size, tmp_buf, copy_size);
      read_size += copy_size;

      rc = forward(copy_size);
    }
  }

  return rc;
}

RC RingBuffer::buffer(const char *&buf, int32_t &read_size)
{
  const int32_t size = this->size();
  if (size == 0) {
    buf       = buffer_.data();
    read_size = 0;
    return RC::SUCCESS;
  }

  const int32_t read_pos = this->read_pos();
  if (read_pos < write_pos_) {
    read_size = write_pos_ - read_pos;
  } else {
    read_size = capacity() - read_pos;
  }
  buf = buffer_.data() + read_pos;
  return RC::SUCCESS;
}

RC RingBuffer::forward(int32_t size)
{
  if (size <= 0) {
    return RC::INVALID_ARGUMENT;
  }

  if (size > this->size()) {
    LOG_DEBUG("forward size is too large.size=%d, size=%d", size, this->size());
    return RC::INVALID_ARGUMENT;
  }

  data_size_ -= size;
  return RC::SUCCESS;
}

RC RingBuffer::write(const char *data, int32_t size, int32_t &write_size)
{
  if (size < 0) {
    return RC::INVALID_ARGUMENT;
  }

  RC rc      = RC::SUCCESS;
  write_size = 0;
  while (OB_SUCC(rc) && write_size<size &&this->remain()> 0) {

    const int32_t read_pos     = this->read_pos();
    const int32_t tmp_buf_size = (read_pos <= write_pos_) ? (capacity() - write_pos_) : (read_pos - write_pos_);

    const int32_t copy_size = min(size - write_size, tmp_buf_size);
    memcpy(buffer_.data() + write_pos_, data + write_size, copy_size);
    write_size += copy_size;
    write_pos_ = (write_pos_ + copy_size) % capacity();
    data_size_ += copy_size;
  }

  return rc;
}
