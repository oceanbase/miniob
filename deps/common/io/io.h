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

#pragma once

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

int writeToFile(const std::string &fileName, const char *data, uint32_t dataSize, const char *openMode);

/**
 * return the line number which line.strip() isn't empty
 */
int getFileLines(const std::string &fileName, uint64_t &lineNum);

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
int getFileList(
    std::vector<std::string> &fileList, const std::string &path, const std::string &pattern, bool recursive);
int getFileNum(uint64_t &fileNum, const std::string &path, const std::string &pattern, bool recursive);
int getDirList(std::vector<std::string> &dirList, const std::string &path, const std::string &pattern);

int touch(const std::string &fileName);

/**
 * get file size
 */
int getFileSize(const char *filePath, uint64_t &fileLen);

/**
 * @brief 一次性写入所有指定数据
 * 
 * @param fd  写入的描述符
 * @param buf 写入的数据
 * @param size 写入多少数据
 * @return int 0 表示成功，否则返回errno
 */
int writen(int fd, const void *buf, int size);

/**
 * @brief 一次性读取指定长度的数据
 * 
 * @param fd  读取的描述符
 * @param buf 读取到这里
 * @param size 读取的数据长度
 * @return int 返回0表示成功。-1 表示读取到文件尾，并且没有读到size大小数据，其它表示errno
 */
int readn(int fd, void *buf, int size);

}  // namespace common
