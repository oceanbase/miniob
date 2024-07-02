/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <vector>
#include <iostream>
#include <unordered_map>

#include "common/math/simd_util.h"
#include "common/rc.h"
#include "sql/expr/expression.h"

/**
 * @brief 用于hash group by 的哈希表实现，不支持并发访问。
 */
class AggregateHashTable
{
public:
  class Scanner
  {
  public:
    explicit Scanner(AggregateHashTable *hash_table) : hash_table_(hash_table) {}
    virtual ~Scanner() = default;

    virtual void open_scan() = 0;

    /**
     * 通过扫描哈希表，将哈希表中的聚合结果写入 chunk 中。
     */
    virtual RC next(Chunk &chunk) = 0;

    virtual void close_scan(){};

  protected:
    AggregateHashTable *hash_table_;
  };

  /**
   * @brief 将 groups_chunk 和 aggrs_chunk 写入到哈希表中。哈希表中记录了聚合结果。
   */
  virtual RC add_chunk(Chunk &groups_chunk, Chunk &aggrs_chunk) = 0;

  virtual ~AggregateHashTable() = default;
};

class StandardAggregateHashTable : public AggregateHashTable
{
private:
  struct VectorHash
  {
    std::size_t operator()(const std::vector<Value> &vec) const;
  };

  struct VectorEqual
  {
    bool operator()(const std::vector<Value> &lhs, const std::vector<Value> &rhs) const;
  };

public:
  using StandardHashTable = std::unordered_map<std::vector<Value>, std::vector<Value>, VectorHash, VectorEqual>;
  class Scanner : public AggregateHashTable::Scanner
  {
  public:
    explicit Scanner(AggregateHashTable *hash_table) : AggregateHashTable::Scanner(hash_table) {}
    ~Scanner() = default;

    void open_scan() override;

    RC next(Chunk &chunk) override;

  private:
    StandardHashTable::iterator end_;
    StandardHashTable::iterator it_;
  };
  StandardAggregateHashTable(const std::vector<Expression *> aggregations)
  {
    for (auto &expr : aggregations) {
      ASSERT(expr->type() == ExprType::AGGREGATION, "expect aggregate expression");
      auto *aggregation_expr = static_cast<AggregateExpr *>(expr);
      aggr_types_.push_back(aggregation_expr->aggregate_type());
    }
  }

  virtual ~StandardAggregateHashTable() {}

  RC add_chunk(Chunk &groups_chunk, Chunk &aggrs_chunk) override;

  StandardHashTable::iterator begin() { return aggr_values_.begin(); }
  StandardHashTable::iterator end() { return aggr_values_.end(); }

private:
  /// group by values -> aggregate values
  StandardHashTable                aggr_values_;
  std::vector<AggregateExpr::Type> aggr_types_;
};

/**
 * @brief 线性探测哈希表实现
 * @note 当前只支持group by 列为 char/char(4) 类型，且聚合列为单列。
 */
#ifdef USE_SIMD
template <typename V>
class LinearProbingAggregateHashTable : public AggregateHashTable
{
public:
  class Scanner : public AggregateHashTable::Scanner
  {
  public:
    explicit Scanner(AggregateHashTable *hash_table) : AggregateHashTable::Scanner(hash_table) {}
    ~Scanner() = default;

    void open_scan() override;

    RC next(Chunk &chunk) override;

    void close_scan() override;

  private:
    int capacity_   = -1;
    int size_       = -1;
    int scan_pos_   = -1;
    int scan_count_ = 0;
  };

  LinearProbingAggregateHashTable(AggregateExpr::Type aggregate_type, int capacity = DEFAULT_CAPACITY)
      : keys_(capacity, EMPTY_KEY), values_(capacity, 0), capacity_(capacity), aggregate_type_(aggregate_type)
  {}
  virtual ~LinearProbingAggregateHashTable() {}

  RC get(int key, V &value);

  RC iter_get(int pos, int &key, V &value);

  RC add_chunk(Chunk &group_chunk, Chunk &aggr_chunk) override;

  int capacity() { return capacity_; }
  int size() { return size_; }

private:
  /**
   * @brief 将键值对以批量的形式写入哈希表中，这里参考了论文
   * `Rethinking SIMD Vectorization for In-Memory Databases` 中的 `Algorithm 5`。
   * @param input_keys 输入的键数组
   * @param input_values 输入的值数组，与键数组一一对应。
   * @param len 键值对数组的长度
   */
  void add_batch(int *input_keys, V *input_values, int len);

  void aggregate(V *value, V value_to_aggregate);

  void resize();

  void resize_if_need();

private:
  static const int EMPTY_KEY;
  static const int DEFAULT_CAPACITY;

  std::vector<int>    keys_;
  std::vector<V>      values_;
  int                 size_     = 0;
  int                 capacity_ = 0;
  AggregateExpr::Type aggregate_type_;
};
#endif