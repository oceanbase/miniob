/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "sql/optimizer/cascade/property.h"

class PropertySet;
struct PropSetPtrHash
{
  /**
   * TODO: WIP
   */
  std::size_t operator()(PropertySet *const &s) const { return 0; }
};

struct PropSetPtrEq
{
  /**
   * TODO: WIP
   */
  bool operator()(PropertySet *const &t1, PropertySet *const &t2) const { return false; }
};

/**
 * TODO: WIP
 */
class PropertySet
{
public:
  PropertySet()  = default;
  ~PropertySet() = default;

private:
  std::vector<Property *> properties_;
};