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

#ifndef __COMMON_LANG_BITMAP_H__
#define __COMMON_LANG_BITMAP_H__

namespace common {

class Bitmap {
public:
  Bitmap(char *bitmap, int size);

  bool get_bit(int index);
  void set_bit(int index);
  void clear_bit(int index);

  int next_unsetted_bit(int start);
  int next_setted_bit(int start);

private:
  char *bitmap_;
  int size_;
};

}  // namespace common

#endif  // __COMMON_LANG_BITMAP_H__