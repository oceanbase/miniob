/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/util/ob_file_reader.h"
#include <fcntl.h>
#include <unistd.h>
#include "common/lang/filesystem.h"

#include "common/log/log.h"

namespace oceanbase {

ObFileReader::~ObFileReader() { close_file(); }

string ObFileReader::read_pos(uint32_t pos, uint32_t size)
{
  string buf;
  buf.resize(size);
  ssize_t read_size = ::pread(fd_, buf.data(), size, static_cast<off_t>(pos));
  if (read_size != size) {
    LOG_WARN("Failed to read file %s, read_size=%ld, size=%ld", filename_.c_str(), read_size, size);
    return "";
  }
  
  return buf;
}

uint32_t ObFileReader::file_size()
{
  return filesystem::file_size(filename_);
}

unique_ptr<ObFileReader> ObFileReader::create_file_reader(const string &filename)
{
  unique_ptr<ObFileReader> reader(new ObFileReader(filename));
  if (OB_FAIL(reader->open_file())) {
    LOG_WARN("Failed to open file %s", filename.c_str());
    return nullptr;
  }
  return reader;
}

RC ObFileReader::open_file()
{
  RC rc = RC::SUCCESS;
  fd_ = ::open(filename_.c_str(), O_RDONLY);
  if (fd_ < 0) {
    LOG_WARN("Failed to open file %s", filename_.c_str());
    rc = RC::INTERNAL;
  }
  return rc;
}

void ObFileReader::close_file()
{
  ::close(fd_);
}

}  // namespace oceanbase
