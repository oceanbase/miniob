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
// Created by WangYunlai on 2024
//

#include <stdint.h>
#include <stdio.h>
#include <execinfo.h>
#include <inttypes.h>
#include "common/lang/vector.h"
#include <cstdlib>

namespace common {

struct ProcMapSegment
{
  uint64_t  start_address;
  uint64_t  end_address;
};

class ProcMap
{
public:
  ProcMap() = default;
  ~ProcMap() = default;

  int parse();
  int parse_file(const char *file_name);

  const ProcMapSegment *get_segment(const uint64_t address) const;

  uint64_t get_offset(const uint64_t address) const;

private:
  vector<ProcMapSegment> segments_;
};

int ProcMap::parse()
{
  const char *file_name = "/proc/self/maps";
  return parse_file(file_name);
}

int ProcMap::parse_file(const char *file_name)
{
  FILE *fp = fopen(file_name, "r");
  if (fp == nullptr) {
    return -1;
  }

  ProcMapSegment segment;
  char line[1024] = {0};
  uint64_t start, end, inode, offset, major, minor;
  char perms[8];
  char path[257];

  while (fgets(line, sizeof(line), fp) != nullptr) {
    
    int ret = sscanf(line, "%" PRIx64 "-%" PRIx64 " %4s %" PRIx64 " %" PRIx64 ":%" PRIx64 "%" PRIu64 "%255s",
                     &start, &end, perms, &offset, &major, &minor, &inode, path);
    if (ret < 8 || perms[2] != 'x') {
      continue;
    }

    segment.start_address = start;
    segment.end_address = end;

    segments_.push_back(segment);
  }

  fclose(fp);
  return 0;
}

const ProcMapSegment *ProcMap::get_segment(const uint64_t address) const
{
  for (const auto &segment : segments_) {
    if (address >= segment.start_address && address < segment.end_address) {
      return &segment;
    }
  }

  return nullptr;
}

uint64_t ProcMap::get_offset(const uint64_t address) const
{
  const ProcMapSegment *segment = get_segment(address);
  if (segment == nullptr) {
    return address;
  }

  return address - segment->start_address;
}

//////////////////////////////////////////////////////////////////////////
static ProcMap g_proc_map;
int backtrace_init()
{
  return g_proc_map.parse();
}

const char *lbt()
{
  constexpr int buffer_size = 100;
  void         *buffer[buffer_size];

  constexpr int     bt_buffer_size = 8192;
  thread_local char backtrace_buffer[bt_buffer_size];

  int size = backtrace(buffer, buffer_size);

  char **symbol_array = nullptr;
#ifdef LBT_SYMBOLS
  /* 有些环境下，使用addr2line 无法根据地址输出符号 */
  symbol_array = backtrace_symbols(buffer, size);
#endif  // LBT_SYMBOLS

  int offset = 0;
  for (int i = 0; i < size && offset < bt_buffer_size - 1; i++) {
    uint64_t address = reinterpret_cast<uint64_t>(buffer[i]);
    address = g_proc_map.get_offset(address);
    const char *format = (0 == i) ? "0x%lx" : " 0x%lx";
    offset += snprintf(backtrace_buffer + offset, sizeof(backtrace_buffer) - offset, format, address);

    if (symbol_array != nullptr) {
      offset += snprintf(backtrace_buffer + offset, sizeof(backtrace_buffer) - offset, " %s", symbol_array[i]);
    }
  }

  if (symbol_array != nullptr) {
    free(symbol_array);
  }
  return backtrace_buffer;
}

} // namespace common