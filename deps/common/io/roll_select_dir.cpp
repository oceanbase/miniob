/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
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

#include "common/io/roll_select_dir.h"
#include "common/io/io.h"
#include "common/log/log.h"
namespace common {

void RollSelectDir::setBaseDir(std::string baseDir)
{
  mBaseDir = baseDir;

  std::vector<std::string> dirList;
  int rc = getDirList(dirList, mBaseDir, "");
  if (rc) {
    LOG_ERROR("Failed to all subdir entry");
  }

  if (dirList.size() == 0) {
    MUTEX_LOCK(&mMutex);

    mSubdirs.clear();
    mSubdirs.push_back(mBaseDir);
    mPos = 0;
    MUTEX_UNLOCK(&mMutex);

    return;
  }

  MUTEX_LOCK(&mMutex);
  mSubdirs = dirList;
  mPos = 0;
  MUTEX_UNLOCK(&mMutex);
  return;
}

std::string RollSelectDir::select()
{
  std::string ret;

  MUTEX_LOCK(&mMutex);
  ret = mSubdirs[mPos % mSubdirs.size()];
  mPos++;
  MUTEX_UNLOCK(&mMutex);

  return ret;
}

}  // namespace common