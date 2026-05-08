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
// Created by Assistant on 2026/05/07.
//

#include "sql/operator/update_physical_operator.h"
#include "common/log/log.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
#include "storage/field/field.h"
#include "storage/record/record.h"
#include <vector>

UpdatePhysicalOperator::UpdatePhysicalOperator(Table *table, const char *attribute_name, const Value &value)
    : table_(table), attribute_name_(attribute_name), value_(value)
{}

RC UpdatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  unique_ptr<PhysicalOperator> &child = children_[0];

  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  trx_ = trx;
  std::vector<std::pair<Record, Record>> update_records;

  while (OB_SUCC(rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }

    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    // 复制记录数据到 owned record，避免后续 child->close() 释放页面内存导致悬空指针
    Record inplace = row_tuple->record();
    Record old_record;
    RC     copy_rc = old_record.copy_data(inplace.data(), inplace.len());
    if (copy_rc != RC::SUCCESS) {
      LOG_WARN("failed to copy record data. rc=%s", strrc(copy_rc));
      child->close();
      return copy_rc;
    }
    old_record.set_rid(inplace.rid());

    // 获取要更新的字段信息
    const FieldMeta *field_meta = table_->table_meta().field(attribute_name_.c_str());
    if (nullptr == field_meta) {
      LOG_WARN("field not found: %s", attribute_name_.c_str());
      child->close();
      return RC::SCHEMA_FIELD_MISSING;
    }

    int field_offset = field_meta->offset();
    int field_len    = field_meta->len();

    // 复制旧记录并构造要写入的新记录
    Record new_record;
    char  *record_data = (char *)malloc(table_->table_meta().record_size());
    memcpy(record_data, old_record.data(), table_->table_meta().record_size());

    // 更新字段值（对 TEXT/CHARS 做以 '\0' 结尾的安全复制，其他类型按字段长度直接复制）
    const char *value_data = value_.data();
    int         value_len  = value_.length();

    if (field_meta->type() == AttrType::TEXTS || field_meta->type() == AttrType::CHARS) {
      // 对字符串类型字段做安全复制：截断到字段长度-1，写入 '\0' 结束符，
      // 并清零字段剩余空间，避免保留旧数据碎片。
      if (field_len <= 0) {
        // 保守处理：如果元数据长度异常（<=0），则直接按 value_len 复制并加入终止符
        int copy_len = value_len;
        if (copy_len > 0) {
          memcpy(record_data + field_offset, value_data, copy_len);
        }
        record_data[field_offset + copy_len] = '\0';
      } else {
        int max_copy = field_len - 1;  // 留一个字节放终止符
        int copy_len = (value_len < max_copy) ? value_len : max_copy;
        if (copy_len > 0) {
          memcpy(record_data + field_offset, value_data, copy_len);
        }
        // 写入终止符
        record_data[field_offset + copy_len] = '\0';
        // 清零剩余空间，防止旧数据残留
        int remain = field_len - (copy_len + 1);
        if (remain > 0) {
          memset(record_data + field_offset + copy_len + 1, 0, remain);
        }
      }
    } else {
      // 其他类型，按照字段长度直接复制（可能包含二进制数据）
      int copy_len = field_len;
      if (copy_len > 0) {
        // 如果 value_len 小于字段长度，则只复制 value_len，剩余保留原值或置零由上层决定
        int actual = (value_len < copy_len) ? value_len : copy_len;
        memcpy(record_data + field_offset, value_data, actual);
        if (actual < copy_len) {
          // zero the rest to avoid leftover bytes
          memset(record_data + field_offset + actual, 0, copy_len - actual);
        }
      }
    }

    new_record.set_data_owner(record_data, table_->table_meta().record_size());
    new_record.set_rid(old_record.rid());
    update_records.emplace_back(std::move(old_record), std::move(new_record));
  }

  if (rc != RC::RECORD_EOF) {
    LOG_WARN("failed to iterate child operator: %s", strrc(rc));
    child->close();
    return rc;
  }

  rc = child->close();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to close child operator: %s", strrc(rc));
    return rc;
  }

  for (auto &record_pair : update_records) {
    rc = trx_->update_record(table_, record_pair.first, record_pair.second);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to update record: %s", strrc(rc));
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next() { return RC::RECORD_EOF; }

RC UpdatePhysicalOperator::close() { return RC::SUCCESS; }
