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

#ifndef __COMMON_IO_IO_H__
#define __COMMON_IO_IO_H__

#include <string>
#include <vector>

#include "common/defs.h"

namespace common {

/**
 * read data from file fileName, store data to data
 * if success, store file continent to data
 * if fail, return -1 and don't change data
 */
int readFromFile(const std::string &fileName, char *&data, size_t &fileSize);

int writeToFile(const std::string &fileName, const char *data, u32_t dataSize,
                const char *openMode);

/**
 * return the line number which line.strip() isn't empty
 */
int getFileLines(const std::string &fileName, u64_t &lineNum);

/** Get file list from the dir
 * don't care ".", "..", ".****" hidden files
 * just count regular files, don't care directory
 * @param[out]  fileList   file List
 * @param[in]   path       the search path
 * @param[in]   pattern    regex string, if not empty, the file should match
 * list
 * @param[in]   resursion  if this has been set, it will search subdirs
 * @return  0   if success, error code otherwise
 */
int getFileList(std::vector<std::string> &fileList, const std::string &path,
                const std::string &pattern, bool recursive);
int getFileNum(u64_t &fileNum, const std::string &path,
               const std::string &pattern, bool recursive);
int getDirList(std::vector<std::string> &dirList, const std::string &path,
               const std::string &pattern);

int touch(const std::string &fileName);

/**
 * get file size
 */
int getFileSize(const char *filePath, u64_t &fileLen);

} //namespace common
#endif /* __COMMON_IO_IO_H__ */
