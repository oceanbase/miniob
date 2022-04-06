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
// Created by wangyunlai on 2021/5/7.
//

#include "common/lang/bitmap.h"

namespace common {

int find_first_zero(char byte, int start)
{
  for (int i = start; i < 8; i++) {
    if ((byte & (1 << i)) == 0) {
      return i;
    }
  }
  return -1;
}

int find_first_setted(char byte, int start)
{
  for (int i = start; i < 8; i++) {
    if ((byte & (1 << i)) != 0) {
      return i;
    }
  }
  return -1;
}

Bitmap::Bitmap(char *bitmap, int size) : bitmap_(bitmap), size_(size)
{}

bool Bitmap::get_bit(int index)
{
  char bits = bitmap_[index / 8];
  return (bits & (1 << (index % 8))) != 0;
}

void Bitmap::set_bit(int index)
{
  char &bits = bitmap_[index / 8];
  bits |= (1 << (index % 8));
}

void Bitmap::clear_bit(int index)
{
  char &bits = bitmap_[index / 8];
  bits &= ~(1 << (index % 8));
}

int Bitmap::next_unsetted_bit(int start)
{
  int ret = -1;
  int start_in_byte = start % 8;
  for (int iter = start / 8, end = (size_ % 8 == 0 ? size_ / 8 : size_ / 8 + 1); iter <= end; iter++) {
    char byte = bitmap_[iter];
    if (byte != -1) {
      int index_in_byte = find_first_zero(byte, start_in_byte);
      if (index_in_byte >= 0) {
        ret = iter * 8 + index_in_byte;
        break;
      }

      start_in_byte = 0;
    }
  }

  if (ret >= size_) {
    ret = -1;
  }
  return ret;
}

int Bitmap::next_setted_bit(int start)
{
  int ret = -1;
  int start_in_byte = start % 8;
  for (int iter = start / 8, end = (size_ % 8 == 0 ? size_ / 8 : size_ / 8 + 1); iter <= end; iter++) {
    char byte = bitmap_[iter];
    if (byte != 0x00) {
      int index_in_byte = find_first_setted(byte, start_in_byte);
      if (index_in_byte >= 0) {
        ret = iter * 8 + index_in_byte;
        break;
      }

      start_in_byte = 0;
    }
  }

  if (ret >= size_) {
    ret = -1;
  }
  return ret;
}

}  // namespace common