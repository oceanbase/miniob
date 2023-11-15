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
// Created by qiling on 2021/4/13.
//
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "common/log/log.h"
#include "persist.h"

PersistHandler::PersistHandler() {}

PersistHandler::~PersistHandler() { close_file(); }

RC PersistHandler::create_file(const char *file_name)
{
  RC rc = RC::SUCCESS;
  if (file_name == nullptr) {
    LOG_ERROR("Failed to create file, because file name is null.");
    rc = RC::FILE_NAME;
  } else if (!file_name_.empty()) {
    LOG_ERROR("Failed to create %s, because a file is already bound.", file_name);
    rc = RC::FILE_BOUND;
  } else {
    int fd;
    fd = open(file_name, O_RDWR | O_CREAT | O_EXCL, S_IREAD | S_IWRITE);
    if (fd < 0) {
      LOG_ERROR("Failed to create %s, due to %s.", file_name, strerror(errno));
      rc = RC::FILE_CREATE;
    } else {
      file_name_ = file_name;
      close(fd);
      LOG_INFO("Successfully create %s.", file_name);
    }
  }

  return rc;
}

RC PersistHandler::open_file(const char *file_name)
{
  int fd;
  RC  rc = RC::SUCCESS;
  if (file_name == nullptr) {
    if (file_name_.empty()) {
      LOG_ERROR("Failed to open file, because no file name.");
      rc = RC::FILE_NAME;
    } else {
      if ((fd = open(file_name_.c_str(), O_RDWR)) < 0) {
        LOG_ERROR("Failed to open file %s, because %s.", file_name_.c_str(), strerror(errno));
        rc = RC::FILE_OPEN;
      } else {
        file_desc_ = fd;
        LOG_INFO("Successfully open file %s.", file_name_.c_str());
      }
    }
  } else {
    if (!file_name_.empty()) {
      LOG_ERROR("Failed to open file, because a file is already bound.");
      rc = RC::FILE_BOUND;
    } else {
      if ((fd = open(file_name, O_RDWR)) < 0) {
        LOG_ERROR("Failed to open file %s, because %s.", file_name, strerror(errno));
        return RC::FILE_OPEN;
      } else {
        file_name_ = file_name;
        file_desc_ = fd;
        LOG_INFO("Successfully open file %s.", file_name);
      }
    }
  }

  return rc;
}

RC PersistHandler::close_file()
{
  RC rc = RC::SUCCESS;
  if (file_desc_ >= 0) {
    if (close(file_desc_) < 0) {
      LOG_ERROR("Failed to close file %d:%s, error:%s", file_desc_, file_name_.c_str(), strerror(errno));
      rc = RC::FILE_CLOSE;
    } else {
      file_desc_ = -1;
      LOG_INFO("Successfully close file %d:%s.", file_desc_, file_name_.c_str());
    }
  }

  return rc;
}

RC PersistHandler::remove_file(const char *file_name)
{
  RC rc = RC::SUCCESS;

  if (file_name != nullptr) {
    if (remove(file_name) == 0) {
      LOG_INFO("Successfully remove file %s.", file_name);
    } else {
      LOG_ERROR("Failed to remove file %s, error:%s", file_name, strerror(errno));
      rc = RC::FILE_REMOVE;
    }
  } else if (!file_name_.empty()) {
    if (file_desc_ < 0 || (rc = close_file()) == RC::SUCCESS) {
      if (remove(file_name_.c_str()) == 0) {
        LOG_INFO("Successfully remove file %s.", file_name_.c_str());
      } else {
        LOG_ERROR("Failed to remove file %s, error:%s", file_name_.c_str(), strerror(errno));
        rc = RC::FILE_REMOVE;
      }
    }
  }

  return rc;
}

RC PersistHandler::write_file(int size, const char *data, int64_t *out_size)
{
  RC rc = RC::SUCCESS;
  if (file_name_.empty()) {
    LOG_ERROR("Failed to write, because file is not exist.");
    rc = RC::FILE_NOT_EXIST;
  } else if (file_desc_ < 0) {
    LOG_ERROR("Failed to write, because file is not opened.");
    rc = RC::FILE_NOT_OPENED;
  } else {
    int64_t write_size = 0;
    if ((write_size = write(file_desc_, data, size)) != size) {
      LOG_ERROR("Failed to write %d:%s due to %s. Write size: %lld",
          file_desc_, file_name_.c_str(), strerror(errno), write_size);
      rc = RC::FILE_WRITE;
    }
    if (out_size != nullptr) {
      *out_size = write_size;
    }
  }

  return rc;
}

RC PersistHandler::write_at(uint64_t offset, int size, const char *data, int64_t *out_size)
{
  RC rc = RC::SUCCESS;
  if (file_name_.empty()) {
    LOG_ERROR("Failed to write, because file is not exist.");
    rc = RC::FILE_NOT_EXIST;
  } else if (file_desc_ < 0) {
    LOG_ERROR("Failed to write, because file is not opened.");
    rc = RC::FILE_NOT_OPENED;
  } else {
    if (lseek(file_desc_, offset, SEEK_SET) == off_t(-1)) {
      LOG_ERROR("Failed to write %lld of %d:%s due to failed to seek %s.",
          offset, file_desc_, file_name_.c_str(), strerror(errno));
      rc = RC::FILE_SEEK;
    } else {
      int64_t write_size = 0;
      if ((write_size = write(file_desc_, data, size)) != size) {
        LOG_ERROR("Failed to write %llu of %d:%s due to %s. Write size: %lld",
            offset, file_desc_, file_name_.c_str(), strerror(errno), write_size);
        rc = RC::FILE_WRITE;
      }
      if (out_size != nullptr) {
        *out_size = write_size;
      }
    }
  }

  return rc;
}

RC PersistHandler::append(int size, const char *data, int64_t *out_size)
{
  RC rc = RC::SUCCESS;
  if (file_name_.empty()) {
    LOG_ERROR("Failed to write, because file is not exist.");
    rc = RC::FILE_NOT_EXIST;
  } else if (file_desc_ < 0) {
    LOG_ERROR("Failed to append, because file is not opened.");
    rc = RC::FILE_NOT_OPENED;
  } else {
    if (lseek(file_desc_, 0, SEEK_END) == off_t(-1)) {
      LOG_ERROR("Failed to append file %d:%s due to failed to seek: %s.",
        file_desc_, file_name_.c_str(), strerror(errno));
      rc = RC::FILE_SEEK;
    } else {
      int64_t write_size = 0;
      if ((write_size = write(file_desc_, data, size)) != size) {
        LOG_ERROR("Failed to append file %d:%s due to %s. Write size: %lld",
            file_desc_, file_name_.c_str(), strerror(errno), write_size);
        rc = RC::FILE_WRITE;
      }
      if (out_size != nullptr) {
        *out_size = write_size;
      }
    }
  }

  return rc;
}

RC PersistHandler::read_file(int size, char *data, int64_t *out_size)
{
  RC rc = RC::SUCCESS;
  if (file_name_.empty()) {
    LOG_ERROR("Failed to read, because file is not exist.");
    rc = RC::FILE_NOT_EXIST;
  } else if (file_desc_ < 0) {
    LOG_ERROR("Failed to read, because file is not opened.");
    rc = RC::FILE_NOT_OPENED;
  } else {
    int64_t read_size = 0;
    if ((read_size = read(file_desc_, data, size)) != size) {
      LOG_ERROR("Failed to read file %d:%s due to %s.", file_desc_, file_name_.c_str(), strerror(errno));
      rc = RC::FILE_READ;
    }
    if (out_size != nullptr) {
      *out_size = read_size;
    }
  }

  return rc;
}

RC PersistHandler::read_at(uint64_t offset, int size, char *data, int64_t *out_size)
{
  RC rc = RC::SUCCESS;
  if (file_name_.empty()) {
    LOG_ERROR("Failed to read, because file is not exist.");
    rc = RC::FILE_NOT_EXIST;
  } else if (file_desc_ < 0) {
    LOG_ERROR("Failed to read, because file is not opened.");
    rc = RC::FILE_NOT_OPENED;
  } else {
    if (lseek(file_desc_, offset, SEEK_SET) == off_t(-1)) {
      LOG_ERROR("Failed to read %llu of %d:%s due to failed to seek %s.",
          offset, file_desc_, file_name_.c_str(), strerror(errno));
      return RC::FILE_SEEK;
    } else {
      ssize_t read_size = read(file_desc_, data, size);
      if (read_size == 0) {
        LOG_TRACE("read file touch the end. file name=%s", file_name_.c_str());
      } else if (read_size < 0) {
        LOG_WARN("failed to read file. file name=%s, offset=%lld, size=%d, error=%s",
          file_name_.c_str(), offset, size, strerror(errno));
        rc = RC::FILE_READ;
      } else if (out_size != nullptr) {
        *out_size = read_size;
      }
    }
  }

  return rc;
}

RC PersistHandler::seek(uint64_t offset)
{
  RC rc = RC::SUCCESS;
  if (file_name_.empty()) {
    LOG_ERROR("Failed to seek, because file is not exist.");
    rc = RC::FILE_NOT_EXIST;
  } else if (file_desc_ < 0) {
    LOG_ERROR("Failed to seek, because file is not opened.");
    rc = RC::FILE_NOT_OPENED;
  } else if (lseek(file_desc_, offset, SEEK_SET) == off_t(-1)) {
    LOG_ERROR("Failed to seek %llu of %d:%s due to %s.", offset, file_desc_, file_name_.c_str(), strerror(errno));
    rc = RC::FILE_SEEK;
  }

  return rc;
}
