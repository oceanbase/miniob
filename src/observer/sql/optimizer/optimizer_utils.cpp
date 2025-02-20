/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/optimizer/optimizer_utils.h"

string OptimizerUtils::dump_physical_plan(const unique_ptr<PhysicalOperator>& children)
{
  std::function<void(ostream &, PhysicalOperator *, int, bool, vector<uint8_t> &)> to_string = [&](
    ostream &os, PhysicalOperator *oper, int level, bool last_child, vector<uint8_t> &ends)
  {
    for (int i = 0; i < level - 1; i++) {
      if (ends[i]) {
        os << "  ";
      } else {
        os << "│ ";
      }
    }
    if (level > 0) {
      if (last_child) {
        os << "└─";
        ends[level - 1] = 1;
      } else {
        os << "├─";
      }
    }

    os << oper->name();
    string param = oper->param();
    if (!param.empty()) {
      os << "(" << param << ")";
    }
    os << '\n';

    if (static_cast<int>(ends.size()) < level + 2) {
      ends.resize(level + 2);
    }
    ends[level + 1] = 0;

    vector<unique_ptr<PhysicalOperator>> &children = oper->children();
    const auto                                 size     = static_cast<int>(children.size());
    for (auto i = 0; i < size - 1; i++) {
      to_string(os, children[i].get(), level + 1, false /*last_child*/, ends);
    }
    if (size > 0) {
      to_string(os, children[size - 1].get(), level + 1, true /*last_child*/, ends);
    }
  };
  stringstream ss;
  ss << "OPERATOR(NAME)\n";

  int               level = 0;
  vector<uint8_t> ends;
  ends.push_back(true);
  to_string(ss, children.get(), level, true /*last_child*/, ends);

  return ss.str();
}