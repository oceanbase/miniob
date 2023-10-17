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
#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "common/rc.h"

class PersistHandler 
{
public:
  PersistHandler();
  ~PersistHandler();

  /** 创建一个名称为指定文件名的文件，并将该文件绑定到当前对象 */
  RC create_file(const char *file_name);

  /** 根据文件名打开一个文件并绑定到当前对象，若文件名为空则打开当前文件 */
  RC open_file(const char *file_name = nullptr);

  /** 关闭当前文件 */
  RC close_file();

  /** 删除指定文件，或删除当前文件 */
  RC remove_file(const char *file_name = nullptr);

  /** 在当前文件描述符的位置写入一段数据，并返回实际写入的数据大小out_size */
  RC write_file(int size, const char *data, int64_t *out_size = nullptr);

  /** 在指定位置写入一段数据，并返回实际写入的数据大小out_size */
  RC write_at(uint64_t offset, int size, const char *data, int64_t *out_size = nullptr);

  /** 在文件末尾写入一段数据，并返回实际写入的数据大小out_size */
  RC append(int size, const char *data, int64_t *out_size = nullptr);

  /** 在当前文件描述符的位置读取一段数据，并返回实际读取的数据大小out_size */
  RC read_file(int size, char *data, int64_t *out_size = nullptr);

  /** 在指定位置读取一段数据，并返回实际读取的数据大小out_size */
  RC read_at(uint64_t offset, int size, char *data, int64_t *out_size = nullptr);

  /** 将文件描述符移动到指定位置 */
  RC seek(uint64_t offset);

private:
  std::string file_name_;
  int file_desc_ = -1;
};
