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
#include <sys/stat.h>
#include <regex.h>
#include <dirent.h>
#include <string.h>

#include <vector>

#include "common/defs.h"
#include "common/os/path.h"
#include "common/log/log.h"
namespace common {

std::string getFileName(const std::string &fullPath)
{
  std::string szRt;
  size_t pos;
  try {
    pos = fullPath.rfind(FILE_PATH_SPLIT);
    if (pos != std::string::npos && pos < fullPath.size() - 1) {
      szRt = fullPath.substr(pos + 1, fullPath.size() - pos - 1);
    } else if (pos == std::string::npos) {
      szRt = fullPath;
    } else {
      szRt = "";
    }

  } catch (...) {}
  return szRt;
}

void getFileName(const char *path, std::string &fileName)
{
  // Don't care the last character as FILE_PATH_SPLIT
  const char *endPos = strrchr(path, FILE_PATH_SPLIT);
  if (endPos == NULL) {
    fileName = path;
    return;
  }

  if (strcmp(path, FILE_PATH_SPLIT_STR) == 0) {
    fileName.assign("");
  } else {
    fileName.assign(endPos + 1);
  }

  return;
}

std::string getDirName(const std::string &fullPath)
{
  std::string szRt;
  size_t pos;
  try {
    pos = fullPath.rfind(FILE_PATH_SPLIT);
    if (pos != std::string::npos && pos > 0) {
      szRt = fullPath.substr(0, pos);
    } else if (pos == std::string::npos) {
      szRt = fullPath;
    } else {
      // pos == 0
      szRt = FILE_PATH_SPLIT_STR;
    }

  } catch (...) {}
  return szRt;
}
void getDirName(const char *path, std::string &parent)
{
  // Don't care the last character as FILE_PATH_SPLIT
  const char *endPos = strrchr(path, FILE_PATH_SPLIT);
  if (endPos == NULL) {
    parent = path;
    return;
  }

  if (endPos == path) {
    parent.assign(path, 1);
  } else {
    parent.assign(path, endPos - path);
  }

  return;
}

std::string getFilePath(const std::string &fullPath)
{
  std::string szRt;
  size_t pos;
  try {
    pos = fullPath.rfind("/");
    if (pos != std::string::npos) {
      szRt = fullPath.substr(0, pos);
    } else if (pos == std::string::npos) {
      szRt = fullPath;
    } else {
      szRt = "";
    }

  } catch (...) {}
  return szRt;
}

std::string getAboslutPath(const char *path)
{
  std::string aPath(path);
  if (path[0] != '/') {
    const int MAX_SIZE = 256;
    char current_absolute_path[MAX_SIZE];

    if (NULL == getcwd(current_absolute_path, MAX_SIZE)) {}
  }

  return aPath;
}

bool is_directory(const char *path)
{
  struct stat st;
  return (0 == stat(path, &st)) && (st.st_mode & S_IFDIR);
}

bool check_directory(std::string &path)
{
  while (!path.empty() && path.back() == '/')
    path.erase(path.size() - 1, 1);

  int len = path.size();

  if (0 == mkdir(path.c_str(), 0777) || is_directory(path.c_str()))
    return true;

  bool sep_state = false;
  for (int i = 0; i < len; i++) {
    if (path[i] != '/') {
      if (sep_state)
        sep_state = false;
      continue;
    }

    if (sep_state)
      continue;

    path[i] = '\0';
    if (0 != mkdir(path.c_str(), 0777) && !is_directory(path.c_str()))
      return false;

    path[i] = '/';
    sep_state = true;
  }

  if (0 != mkdir(path.c_str(), 0777) && !is_directory(path.c_str()))
    return false;
  return true;
}

int list_file(const char *path, const char *filter_pattern, std::vector<std::string> &files)
{
  regex_t reg;
  if (filter_pattern) {
    const int res = regcomp(&reg, filter_pattern, REG_NOSUB);
    if (res) {
      char errbuf[256];
      regerror(res, &reg, errbuf, sizeof(errbuf));
      LOG_ERROR("regcomp return error. filter pattern %s. errmsg %d:%s", filter_pattern, res, errbuf);
      return -1;
    }
  }

  DIR *pdir = opendir(path);
  if (!pdir) {
    if (filter_pattern)
      regfree(&reg);
    LOG_ERROR("open directory failure. path %s, errmsg %d:%s", path, errno, strerror(errno));
    return -1;
  }

  files.clear();

  // readdir_r is deprecated in some systems, so we use readdir instead
  // as readdir is not thread-safe, it is better to use C++ directory
  // TODO
  struct dirent *pentry;
  char tmp_path[PATH_MAX];
  while ((pentry = readdir(pdir)) != NULL) {
    if ('.' == pentry->d_name[0])  // 跳过./..文件和隐藏文件
      continue;

    snprintf(tmp_path, sizeof(tmp_path), "%s/%s", path, pentry->d_name);
    if (is_directory(tmp_path))
      continue;

    if (!filter_pattern || 0 == regexec(&reg, pentry->d_name, 0, NULL, 0))
      files.push_back(pentry->d_name);
  }

  if (filter_pattern)
    regfree(&reg);

  closedir(pdir);
  return files.size();
}

}  // namespace common
