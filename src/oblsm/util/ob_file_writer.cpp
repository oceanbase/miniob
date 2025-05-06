/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/util/ob_file_writer.h"

namespace oceanbase {

ObFileWriter::~ObFileWriter() { close_file(); }

RC ObFileWriter::write(const string_view &data)
{
  RC rc = RC::SUCCESS;
  file_  << data;
  if (!file_.good()) {
    rc = RC::IOERR_WRITE;
  }
  return rc;
}

RC ObFileWriter::flush()
{
  RC rc = RC::SUCCESS;
  file_.flush();
  if (!file_.good()) {
    rc = RC::IOERR_SYNC;
  }
  return rc;
}

RC ObFileWriter::open_file()
{
  RC rc = RC::SUCCESS;
  if (file_.is_open()) {
    return rc;
  }
  if (append_) {
    file_.open(filename_, std::ios::app | std::ios::binary);
  } else {
    file_.open(filename_, std::ios::out | std::ios::trunc | std::ios::binary);
  }
  if (!file_.good()) {
    rc = RC::IOERR_OPEN;
  }
  return rc;
}

void ObFileWriter::close_file()
{
  if (file_.is_open()) {
    file_.flush();
    file_.close();
  }
}

unique_ptr<ObFileWriter> ObFileWriter::create_file_writer(const string &filename, bool append)
{
  unique_ptr<ObFileWriter> writer(new ObFileWriter(filename, append));
  if (writer->open_file() != RC::SUCCESS) {
    return nullptr;
  }
  return writer;
}

}  // namespace oceanbase
