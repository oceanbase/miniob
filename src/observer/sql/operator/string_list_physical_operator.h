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
// Created by WangYunlai on 2022/11/18.
//

#pragma once

#include <vector>
#include "sql/operator/physical_operator.h"

/**
 * @brief 字符串列表物理算子
 * @ingroup PhysicalOperator
 * @details 用于将字符串列表转换为物理算子,为了方便实现的接口，比如help命令
 */
class StringListPhysicalOperator : public PhysicalOperator
{
public:
  StringListPhysicalOperator()
  {}

  virtual ~StringListPhysicalOperator() = default;

  template <typename InputIt>
  void append(InputIt begin, InputIt end)
  {
    strings_.emplace_back(begin, end);
  }

  void append(std::initializer_list<std::string> init)
  {
    strings_.emplace_back(init);
  }

  template <typename T>
  void append(const T &v)
  {
    strings_.emplace_back(1, v);
  }

  PhysicalOperatorType type() const override
  {
    return PhysicalOperatorType::STRING_LIST;
  }

  RC open(Trx *) override
  {
    return RC::SUCCESS;
  }

  RC next() override
  {
    if (!started_) {
      started_ = true;
      iterator_ = strings_.begin();
    } else if (iterator_ != strings_.end()) {
      ++iterator_;
    }
    return iterator_ == strings_.end() ? RC::RECORD_EOF : RC::SUCCESS;
  }

  virtual RC close() override
  {
    iterator_ = strings_.end();
    return RC::SUCCESS;
  }

  virtual Tuple *current_tuple() override
  {
    if (iterator_ == strings_.end()) {
      return nullptr;
    }

    const StringList &string_list = *iterator_;
    std::vector<Value> cells;
    for (const std::string &s : string_list) {

      Value value;
      value.set_string(s.c_str());
      cells.push_back(value);
    }
    tuple_.set_cells(cells);
    return &tuple_;
  }

private:
  using StringList = std::vector<std::string>;
  using StringListList = std::vector<StringList>;
  StringListList strings_;
  StringListList::iterator iterator_;
  bool started_ = false;
  ValueListTuple tuple_;
};
