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
// Created by wangyunlai.wyl on 2024/02/04
//

#include <filesystem>
#include <sstream>

#include "storage/clog/log_entry.h"
#include "storage/clog/log_file.h"
#include "storage/buffer/buffer_pool_log.h"
#include "storage/record/record_log.h"
#include "storage/index/bplus_tree_log_entry.h"
#include "storage/trx/mvcc_trx_log.h"
#include "common/lang/serializer.h"

using namespace std;
using namespace common;
using namespace bplus_tree;

class LogEntryStringifier
{
public:
  string to_string(const LogEntry &entry) const
  {
    stringstream ss;
    ss << entry.header().to_string() << ", ";
    switch (entry.module().id()) {
      case LogModule::Id::BUFFER_POOL: {
        if (entry.payload_size() < static_cast<int32_t>(sizeof(BufferPoolLogEntry))) {
          ss << "invalid buffer pool log entry. "
             << "payload size = " << entry.payload_size() << ", expected size = " << sizeof(BufferPoolLogEntry);
        } else {
          auto *bp_entry = reinterpret_cast<const BufferPoolLogEntry *>(entry.data());
          ss << bp_entry->to_string();
        }
      } break;
      case LogModule::Id::RECORD_MANAGER: {
        if (entry.payload_size() < RecordLogHeader::SIZE) {
          ss << "invalid record log entry. "
             << "payload size = " << entry.payload_size() << ", expected size = " << RecordLogHeader::SIZE;
        } else {
          auto *record_log_header = reinterpret_cast<const RecordLogHeader *>(entry.data());
          ss << record_log_header->to_string();
        }
      } break;
      case LogModule::Id::BPLUS_TREE: {
        ss << BplusTreeLogger::log_entry_to_string(entry);
      } break;

      case LogModule::Id::TRANSACTION: {
        auto *header = reinterpret_cast<const MvccTrxLogHeader *>(entry.data());

        MvccTrxLogOperation operation_type(header->operation_type);
        if (operation_type.type() == MvccTrxLogOperation::Type::INSERT_RECORD ||
            operation_type.type() == MvccTrxLogOperation::Type::DELETE_RECORD) {
          auto *record_log_header = reinterpret_cast<const MvccTrxRecordLogEntry *>(entry.data());
          ss << record_log_header->to_string();
        } else if (operation_type.type() == MvccTrxLogOperation::Type::COMMIT) {
          auto *commit_log = reinterpret_cast<const MvccTrxCommitLogEntry *>(entry.data());
          ss << commit_log->to_string();
        } else {
          ss << header->to_string();
        }
      } break;

      default: {
        ss << "unknown log entry. "
           << "module id = " << entry.module().index();
      } break;
    }
    return ss.str();
  }
};

void dump_file(const filesystem::path &filepath)
{
  LogFileReader log_file;
  RC            rc = log_file.open(filepath.c_str());
  if (OB_FAIL(rc)) {
    printf("failed to open log file. filename = %s, rc = %s\n", filepath.c_str(), strrc(rc));
    return;
  }

  LogEntryStringifier stringifier;

  printf("begin dump file %s\n", filepath.c_str());

  rc = log_file.iterate([&stringifier](const LogEntry &entry) -> RC {
    printf("%s\n", stringifier.to_string(entry).c_str());
    return RC::SUCCESS;
  });

  if (OB_FAIL(rc)) {
    printf("failed to iterate log file. filename = %s, rc = %s\n", filepath.c_str(), strrc(rc));
    return;
  }

  printf("end dump file %s\n", filepath.c_str());

  log_file.close();
}

void dump_directory(const filesystem::path &directory)
{
  LogFileManager log_file_manager;
  RC             rc = log_file_manager.init(directory.c_str(), 1);
  if (OB_FAIL(rc)) {
    printf("failed to init log file manager. rc = %s\n", strrc(rc));
    return;
  }

  vector<string> filenames;
  rc = log_file_manager.list_files(filenames, 0);
  if (OB_FAIL(rc)) {
    printf("failed to list log files. directory = %s, rc = %s\n", directory.c_str(), strrc(rc));
    return;
  }

  LogEntryStringifier stringifier;

  for (const auto &filename : filenames) {
    LogFileReader log_file;
    rc = log_file.open(filename.c_str());
    if (OB_FAIL(rc)) {
      printf("failed to open log file. filename = %s, rc = %s\n", filename.c_str(), strrc(rc));
      return;
    }

    printf("begin dump file %s\n", filename.c_str());

    rc = log_file.iterate([&stringifier](const LogEntry &entry) -> RC {
      printf("%s\n", stringifier.to_string(entry).c_str());
      return RC::SUCCESS;
    });

    if (OB_FAIL(rc)) {
      printf("failed to iterate log file. filename = %s, rc = %s\n", filename.c_str(), strrc(rc));
      return;
    }

    printf("end dump file %s\n", filename.c_str());

    log_file.close();
  }
}

void dump(const char *arg)
{
  filesystem::path path(arg);
  if (filesystem::is_directory(path)) {
    dump_directory(path);
  } else if (filesystem::is_regular_file(path)) {
    dump_file(path);
  } else {
    printf("invalid file or directory name: %s\n", arg);
  }
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("please give me a clog file or directory name\n");
    return 1;
  }

  dump(argv[1]);
  return 0;
}
