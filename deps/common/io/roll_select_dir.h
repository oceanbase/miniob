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
// Created by Longda on 2010
//

#ifndef __COMMON_IO_ROLL_SELECT_DIR__
#define __COMMON_IO_ROLL_SELECT_DIR__

#include <map>
#include <string>
#include <vector>

#include "common/defs.h"
#include "common/io/select_dir.h"
#include "common/lang/mutex.h"
namespace common {

class RollSelectDir : public SelectDir {
public:
  RollSelectDir()
  {
    MUTEX_INIT(&mMutex, NULL);
  }
  ~RollSelectDir()
  {
    MUTEX_DESTROY(&mMutex);
  }

public:
  /**
   * inherit from CSelectDir
   */
  std::string select();
  void setBaseDir(std::string baseDir);

public:
  std::string mBaseDir;
  std::vector<std::string> mSubdirs;
  pthread_mutex_t mMutex;
  uint32_t mPos;
};

}  // namespace common
#endif /* __COMMON_IO_ROLL_SELECT_DIR__ */
