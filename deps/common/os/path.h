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
// Created by Longda on 2021/3/27.
//

#ifndef __COMMON_OS_PATH_H__
#define __COMMON_OS_PATH_H__

#include <string>
namespace common {


/**
 * get file name from full path
 * example
 * input: /test/happy/  --> return : ""
 * input: /test/happy   --> return : happy
 * input: test/happy    --> return : happy
 * input: happy         --> return : happy
 * input: ""            --> return : ""
 */
std::string getFileName(const std::string &fullPath);
void getFileName(const char *path, std::string &fileName);

/**
 * get file path from full path
 * example
 * input: /test/happy   --> return : /test
 * input: test/happy    --> return : test
 * input: happy         --> return : happy
 * input: ""            --> return : ""
 */
std::string getFilePath(const std::string &fullPath);
void getDirName(const char *path, std::string &parent);

/**
 *  Get absolute path
 * input: path
 * reutrn absolutely path
 */
std::string getAboslutPath(const char *path);

/**
 * 判断给定目录是否是文件夹
 */
bool is_directory(const char *path);

/**
 * 判断指定路径是否存在并且是文件夹
 * 如果不存在将会逐级创建
 * @return 创建失败，或者不是文件夹将会返回失败
 */
bool check_directory(std::string &path);

/**
 * 列出指定文件夹下符合指定模式的所有文件
 * @param filter_pattern  示例 ^miniob.*bin$
 * @return 成功返回找到的文件个数，否则返回-1
 */
int list_file(const char *path, const char *filter_pattern, std::vector<std::string> &files); // io/io.h::getFileList

} //namespace common
#endif //__COMMON_OS_PATH_H__
