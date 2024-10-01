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
// Created by wangyunlai on 2024/01/30
//

#include <fcntl.h>

#include "common/lang/string_view.h"
#include "common/lang/charconv.h"
#include "common/log/log.h"
#include "storage/clog/log_file.h"
#include "storage/clog/log_entry.h"
#include "common/io/io.h"

using namespace common;

RC LogFileReader::open(const char *filename)
{
  filename_ = filename;

  fd_ = ::open(filename, O_RDONLY);
  if (fd_ < 0) {
    LOG_WARN("open file failed. filename=%s, error=%s", filename, strerror(errno));
    return RC::FILE_OPEN;
  }

  LOG_INFO("open file success. filename=%s, fd=%d", filename, fd_);
  return RC::SUCCESS;
}

RC LogFileReader::close()
{
  if (fd_ < 0) {
    return RC::FILE_NOT_OPENED;
  }

  ::close(fd_);
  fd_ = -1;
  return RC::SUCCESS;
}

RC LogFileReader::iterate(function<RC(LogEntry &)> callback, LSN start_lsn /*=0*/)
{
  RC rc = skip_to(start_lsn);
  if (OB_FAIL(rc)) {
    return rc;
  }

  LogHeader header;
  while (true) {
    int ret = readn(fd_, reinterpret_cast<char *>(&header), LogHeader::SIZE);
    if (0 != ret) {
      if (-1 == ret) {
        // EOF
        break;
      }
      LOG_WARN("read file failed. filename=%s, ret = %d, error=%s", filename_.c_str(), ret, strerror(errno));
      return RC::IOERR_READ;
    }

    if (header.size < 0 || header.size > LogEntry::max_payload_size()) {
      LOG_WARN("invalid log entry size. filename=%s, size=%d", filename_.c_str(), header.size);
      return RC::IOERR_READ;
    }

    vector<char> data(header.size);
    ret = readn(fd_, data.data(), header.size);
    if (0 != ret) {
      LOG_WARN("read file failed. filename=%s, size=%d, ret=%d, error=%s", filename_.c_str(), header.size, ret, strerror(errno));
      return RC::IOERR_READ;
    }

    LogEntry entry;
    entry.init(header.lsn, LogModule(header.module_id), std::move(data));
    rc = callback(entry);
    if (OB_FAIL(rc)) {
      LOG_INFO("iterate log entry failed. entry=%s, rc=%s", entry.to_string().c_str(), strrc(rc));
      return rc;
    }
    LOG_TRACE("redo log iterate entry success. entry=%s", entry.to_string().c_str());
  }

  return RC::SUCCESS;
}

RC LogFileReader::skip_to(LSN start_lsn)
{
  if (fd_ < 0) {
    return RC::FILE_NOT_OPENED;
  }

  off_t pos = lseek(fd_, 0, SEEK_SET);
  if (off_t(-1) == pos) {
    LOG_WARN("seek file failed. seek to the beginning. filename=%s, error=%s", filename_.c_str(), strerror(errno));
    return RC::IOERR_SEEK;
  }

  LogHeader header;
  while (true) {
    int ret = readn(fd_, reinterpret_cast<char *>(&header), LogHeader::SIZE);
    if (0 != ret) {
      if (-1 == ret) {
        // EOF
        break;
      }
      LOG_WARN("read file failed. filename=%s, ret = %d, error=%s", filename_.c_str(), ret, strerror(errno));
      return RC::IOERR_READ;
    }

    if (header.lsn >= start_lsn) {
      off_t pos = lseek(fd_, -LogHeader::SIZE, SEEK_CUR);
      if (off_t(-1) == pos) {
        LOG_WARN("seek file failed. skip back log header. filename=%s, error=%s", filename_.c_str(), strerror(errno));
        return RC::IOERR_SEEK;
      }
      break;
    }

    if (header.size < 0 || header.size > LogEntry::max_payload_size()) {
      LOG_WARN("invalid log entry size. filename=%s, size=%d", filename_.c_str(), header.size);
      return RC::IOERR_READ;
    }

    pos = lseek(fd_, header.size, SEEK_CUR);
    if (off_t(-1) == pos) {
      LOG_WARN("seek file failed. skip log entry payload. filename=%s, error=%s", filename_.c_str(), strerror(errno));
      return RC::IOERR_SEEK;
    }
  }

  return RC::SUCCESS;
}
////////////////////////////////////////////////////////////////////////////////
// LogFileWriter
LogFileWriter::~LogFileWriter()
{
  (void)this->close();
}

RC LogFileWriter::open(const char *filename, int end_lsn)
{
  if (fd_ >= 0) {
    return RC::FILE_OPEN;
  }

  filename_ = filename;
  end_lsn_ = end_lsn;

  fd_ = ::open(filename, O_WRONLY | O_APPEND | O_CREAT | O_SYNC, 0644);
  if (fd_ < 0) {
    LOG_WARN("open file failed. filename=%s, error=%s", filename, strerror(errno));
    return RC::FILE_OPEN;
  }

  LOG_INFO("open file success. filename=%s, fd=%d", filename, fd_);
  return RC::SUCCESS;
}

RC LogFileWriter::close()
{
  if (fd_ < 0) {
    return RC::FILE_NOT_OPENED;
  }

  ::close(fd_);
  fd_ = -1;
  return RC::SUCCESS;
}

RC LogFileWriter::write(LogEntry &entry)
{
  // 一个日志文件写的日志条数是有限制的
  if (entry.lsn() > end_lsn_) {
    return RC::LOG_FILE_FULL;
  }

  if (fd_ < 0) {
    return RC::FILE_NOT_OPENED;
  }

  if (entry.lsn() <= last_lsn_) {
    LOG_WARN("write log entry failed. lsn is too small. filename=%s, last_lsn=%ld, entry=%s", 
             filename_.c_str(), last_lsn_, entry.to_string().c_str());
    return RC::INVALID_ARGUMENT;
  }

  /// WARNING 这里需要处理日志写一半的情况
  /// 日志只写成功一部分到文件中非常难处理
  int ret = writen(fd_, reinterpret_cast<const char *>(&entry.header()), LogHeader::SIZE);
  if (0 != ret) {
    LOG_WARN("write log entry header failed. filename=%s, ret = %d, error=%s, entry=%s", 
             filename_.c_str(), ret, strerror(errno), entry.to_string().c_str());
    return RC::IOERR_WRITE;
  }

  ret = writen(fd_, entry.data(), entry.payload_size());
  if (0 != ret) {
    LOG_WARN("write log entry payload failed. filename=%s, ret = %d, error=%s, entry=%s", 
             filename_.c_str(), ret, strerror(errno), entry.to_string().c_str());
    return RC::IOERR_WRITE;
  }

  last_lsn_ = entry.lsn();
  LOG_TRACE("write log entry success. filename=%s, entry=%s", filename_.c_str(), entry.to_string().c_str());
  return RC::SUCCESS;
}

bool LogFileWriter::valid() const
{
  return fd_ >= 0;
}

bool LogFileWriter::full() const
{
  return last_lsn_ >= end_lsn_;
}

string LogFileWriter::to_string() const
{
  return filename_;
}

////////////////////////////////////////////////////////////////////////////////
// LogFileManager

RC LogFileManager::init(const char *directory, int max_entry_number_per_file)
{
  directory_ = filesystem::absolute(filesystem::path(directory));
  max_entry_number_per_file_ = max_entry_number_per_file;

  // 检查目录是否存在，不存在就创建出来
  if (!filesystem::is_directory(directory_)) {
    LOG_INFO("directory is not exist. directory=%s", directory_.c_str());

    error_code ec;
    bool ret = filesystem::create_directories(directory_, ec);
    if (!ret) {
      LOG_WARN("create directory failed. directory=%s, error=%s", directory_.c_str(), ec.message().c_str());
      return RC::FILE_CREATE;
    }
  }

  // 列出所有的日志文件
  for (const filesystem::directory_entry &dir_entry : filesystem::directory_iterator(directory_)) {
    if (!dir_entry.is_regular_file()) {
      continue;
    }

    string filename = dir_entry.path().filename().string();
    LSN lsn = 0;
    RC rc = get_lsn_from_filename(filename, lsn);
    if (OB_FAIL(rc)) {
      LOG_TRACE("invalid log file name. filename=%s", filename.c_str());
      continue;
    }

    if (log_files_.find(lsn) != log_files_.end()) {
      LOG_TRACE("duplicate log file. filename1=%s, filename2=%s", 
                filename.c_str(), log_files_.find(lsn)->second.filename().c_str());
      continue;
    }

    log_files_.emplace(lsn, dir_entry.path());
  }

  LOG_INFO("init log file manager success. directory=%s, log files=%d", 
           directory_.c_str(), static_cast<int>(log_files_.size()));
  return RC::SUCCESS;
}

RC LogFileManager::get_lsn_from_filename(const string &filename, LSN &lsn)
{
  if (!filename.starts_with(file_prefix_) || !filename.ends_with(file_suffix_)) {
    return RC::INVALID_ARGUMENT;
  }

  string_view lsn_str(filename.data() + strlen(file_prefix_), filename.length() - strlen(file_suffix_) - strlen(file_prefix_));
  from_chars_result result = from_chars(lsn_str.data(), lsn_str.data() + lsn_str.size(), lsn);
  if (result.ec != errc()) {
    LOG_TRACE("invalid log file name: cannot calc lsn. filename=%s, error=%s", 
              filename.c_str(), strerror(static_cast<int>(result.ec)));
    return RC::INVALID_ARGUMENT;
  }

  return RC::SUCCESS;
}

RC LogFileManager::list_files(vector<string> &files, LSN start_lsn)
{
  files.clear();

  // 这里的代码是AI自动生成的
  // 其实写的不好，我们只需要找到比start_lsn相等或者小的第一个日志文件就可以了
  for (auto &file : log_files_) {
    if (file.first + max_entry_number_per_file_ - 1 >= start_lsn) {
      files.emplace_back(file.second.string());
    }
  }

  return RC::SUCCESS;
}

RC LogFileManager::last_file(LogFileWriter &file_writer)
{
  if (log_files_.empty()) {
    return next_file(file_writer);
  }

  file_writer.close();

  auto last_file_item = log_files_.rbegin();
  return file_writer.open(last_file_item->second.c_str(), 
                          last_file_item->first + max_entry_number_per_file_ - 1);
}

RC LogFileManager::next_file(LogFileWriter &file_writer)
{
  file_writer.close();

  LSN lsn = 0;
  if (!log_files_.empty()) {
    lsn = log_files_.rbegin()->first + max_entry_number_per_file_;
  }

  string filename = file_prefix_ + std::to_string(lsn) + file_suffix_;
  filesystem::path file_path = directory_ / filename;
  log_files_.emplace(lsn, file_path);

  return file_writer.open(file_path.c_str(), lsn + max_entry_number_per_file_ - 1);
}
