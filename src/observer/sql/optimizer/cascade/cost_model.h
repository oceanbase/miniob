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

class Memo;
class GroupExpr;
/**
 * @brief cost model in cost-based optimization(CBO)
 */
class CostModel
{
private:
  // reference columbia optimizer
  double CPU_OP      = 0.00002;
  double HASH_COST   = 0.00002;
  double HASH_PROBE  = 0.00001;
  double INDEX_PROBE = 0.00001;
  double IO          = 0.03;

public:
  // TODO: support user-defined
  CostModel(){};

  inline double cpu_op() { return CPU_OP; }

  ///< cpu cost of building hash table
  inline double hash_cost() { return HASH_COST; }

  ///< cpu cost of finding hash bucket
  inline double hash_probe() { return HASH_PROBE; }

  ///< cpu cost of finding index
  inline double index_probe() { return INDEX_PROBE; }

  ///< i/o cost
  inline double io() { return IO; }

  double calculate_cost(Memo *memo, GroupExpr *gexpr);
};