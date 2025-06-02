/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/operator/update_physical_operator.h"
#include "common/log/log.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"

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

  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next()
{
  RC rc = RC::SUCCESS;
  if (children_.empty()) {
    return RC::RECORD_EOF;
  }

  PhysicalOperator *child = children_[0].get();
  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }

    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record &old_record = row_tuple->record();
    
    // 获取要更新的字段的元数据
    const TableMeta &table_meta = table_->table_meta();
    const FieldMeta *field_meta = table_meta.field(attribute_name_);
    if (nullptr == field_meta) {
      LOG_WARN("no such field in table. table=%s, field=%s", table_->name(), attribute_name_);
      return RC::SCHEMA_FIELD_NOT_EXIST;
    }

    // 创建新记录
    Record new_record;
    new_record.copy_data(old_record.data(), old_record.len());
    new_record.set_rid(old_record.rid());

    // 修改记录中的字段值
    char *record_data = new_record.data();
    int field_offset = field_meta->offset();
    
    // 根据字段类型设置新值
    switch (field_meta->type()) {
      case AttrType::INTS: {
        int int_val = value_.get_int();
        memcpy(record_data + field_offset, &int_val, sizeof(int));
      } break;
      case AttrType::FLOATS: {
        float float_val = value_.get_float();
        memcpy(record_data + field_offset, &float_val, sizeof(float));
      } break;
      case AttrType::CHARS: {
        const char *str_val = value_.get_string().c_str();
        int len = strlen(str_val);
        if (len > field_meta->len()) {
          LOG_WARN("string value too long. field=%s, len=%d, max_len=%d", 
                   attribute_name_, len, field_meta->len());
          return RC::INVALID_ARGUMENT;
        }
        memset(record_data + field_offset, 0, field_meta->len());
        memcpy(record_data + field_offset, str_val, len);
      } break;
      default: {
        LOG_WARN("unsupported field type: %d", field_meta->type());
        return RC::UNSUPPORTED;
      }
    }

    // 执行更新操作：先删除旧记录，再插入新记录
    rc = trx_->delete_record(table_, old_record);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to delete old record: %s", strrc(rc));
      return rc;
    }

    rc = trx_->insert_record(table_, new_record);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to insert new record: %s", strrc(rc));
      return rc;
    }
  }

  return RC::RECORD_EOF;
}

RC UpdatePhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
} 