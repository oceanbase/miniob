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
#include "common/lang/unordered_map.h"
#include "common/lang/mutex.h"
#include "catalog/table_stats.h"

/**
 * @class Catalog
 * @brief Store metadata, such as table statistics.
 *
 * The Catalog class provides methods to access and update metadata related to
 * tables, such as table statistics. It is designed as a singleton to ensure
 * a single point of management for the metadata.
 */
class Catalog
{

public:
  Catalog() = default;

  ~Catalog() = default;

  Catalog(const Catalog &) = delete;

  Catalog &operator=(const Catalog &) = delete;

  /**
   * @brief Retrieves table statistics for a given table_id.
   *
   * @param table_id The identifier of the table for which statistics are requested.
   * @return A constant reference to the TableStats of the specified table_id.
   */
  const TableStats &get_table_stats(int table_id);

  /**
   * @brief Updates table statistics for a given table.
   *
   * @param table_id The identifier of the table for which statistics are updated.
   * @param table_stats The new table statistics to be set.
   */
  void update_table_stats(int table_id, const TableStats &table_stats);

  /**
   * @brief Gets the singleton instance of the Catalog.
   *
   * This method ensures there is only one instance of the Catalog.
   *
   * @return A reference to the singleton instance of the Catalog.
   */
  static Catalog &get_instance()
  {
    static Catalog instance;
    return instance;
  }

private:
  mutex mutex_;

  /**
   * @brief A map storing the table statistics indexed by table_id.
   *
   * This map is currently not persisted and its persistence is planned via a system table.
   */
  unordered_map<int, TableStats> table_stats_;  ///< Table statistics storage.
};