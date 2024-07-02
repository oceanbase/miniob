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
// Created by wangyunlai on 2024/01/31
//

#include <sstream>
#include "storage/clog/log_entry.h"
#include "common/log/log.h"

////////////////////////////////////////////////////////////////////////////////
// struct LogHeader

const int32_t LogHeader::SIZE = sizeof(LogHeader);

string LogHeader::to_string() const
{
  stringstream ss;
  ss << "lsn=" << lsn 
     << ", size=" << size 
     << ", module_id=" << module_id << ":" << LogModule(module_id).name();

  return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
// class LogEntry
LogEntry::LogEntry()
{
  header_.lsn = 0;
  header_.size = 0;
}

LogEntry::LogEntry(LogEntry &&other)
{
  header_ = other.header_;
  data_ = std::move(other.data_);

  other.header_.lsn = 0;
  other.header_.size = 0;
}

LogEntry &LogEntry::operator=(LogEntry &&other)
{
  if (this == &other) {
    return *this;
  }

  header_ = other.header_;
  data_ = std::move(other.data_);

  other.header_.lsn = 0;
  other.header_.size = 0;

  return *this;
}

RC LogEntry::init(LSN lsn, LogModule::Id module_id, vector<char> &&data)
{
  return init(lsn, LogModule(module_id), std::move(data));
}

RC LogEntry::init(LSN lsn, LogModule module, vector<char> &&data)
{
  if (static_cast<int32_t>(data.size()) > max_payload_size()) {
    LOG_DEBUG("log entry size is too large. size=%d, max_payload_size=%d", data.size(), max_payload_size());
    return RC::INVALID_ARGUMENT;
  }

  header_.lsn = lsn;
  header_.module_id = module.index();
  header_.size = static_cast<int32_t>(data.size());
  data_ = std::move(data);
  return RC::SUCCESS;
}

string LogEntry::to_string() const
{
  return header_.to_string();
}
