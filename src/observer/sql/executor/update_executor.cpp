
#include "event/sql_event.h"
#include "event/session_event.h"
#include "sql/stmt/update_stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "session/session.h"
#include "storage/table/table.h"
#include "storage/table/table_meta.h"
#include "storage/trx/trx.h"
#include "common/log/log.h"
#include "storage/record/record.h"
#include "storage/record/record_scanner.h"
#include "storage/common/condition_filter.h"


RC update_execute(SQLStageEvent *sql_event) {
  auto *update_stmt = static_cast<UpdateStmt *>(sql_event->stmt());
  Table *table = update_stmt->table();
  Value *values = update_stmt->values();
  int value_amount = update_stmt->value_amount();
  FilterStmt *filter_stmt = update_stmt->filter_stmt();
  Session *session = sql_event->session_event()->session();
  Trx *trx = session ? session->current_trx() : nullptr;

  // 这里只实现单字段 update
  if (value_amount != 1) {
    LOG_ERROR("Only single field update is supported");
    return RC::UNIMPLEMENTED;
  }

  const std::string &update_field_name = update_stmt->update_field_name();
  const TableMeta &table_meta = table->table_meta();
  const FieldMeta *field = table_meta.field(update_field_name.c_str());
  if (!field) {
    LOG_ERROR("Field not found: %s", update_field_name.c_str());
    return RC::SCHEMA_FIELD_MISSING;
  }

  // 遍历表，找到需要更新的记录
  RecordScanner *scanner = nullptr;
  RC rc = table->get_record_scanner(scanner, trx, ReadWriteMode::READ_WRITE);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to get record scanner. rc=%d", rc);
    return rc;
  }
  int update_count = 0;
  Record record;
  while (scanner->next(record) == RC::SUCCESS) {
    // 判断是否满足 where 条件
    bool pass = true;
    if (filter_stmt) {
      // 只支持 AND 关系
      for (auto unit : filter_stmt->filter_units()) {
        const FilterObj &left = unit->left();
        const FilterObj &right = unit->right();
        Value left_val, right_val;
        if (left.is_attr) {
          const FieldMeta *fm = left.field.meta();
          // set the value type first so set_data can interpret raw bytes correctly
          left_val.set_type(fm->type());
          left_val.set_data(record.data() + fm->offset(), fm->len());
        } else {
          left_val = left.value;
        }
        if (right.is_attr) {
          const FieldMeta *fm = right.field.meta();
          // set the value type first so set_data can interpret raw bytes correctly
          right_val.set_type(fm->type());
          right_val.set_data(record.data() + fm->offset(), fm->len());
        } else {
          right_val = right.value;
        }
        // 只支持等值判断，可扩展
        if (unit->comp() == CompOp::EQUAL_TO) {
          if (left_val.compare(right_val) != 0) {
            pass = false;
            break;
          }
        } else {
          // 其他操作符可按需扩展
          pass = false;
          break;
        }
      }
    }
    if (!pass) continue;
    // 修改字段：构造一份可写的 new_record，再调用 table 的 update 接口
    Record new_record;
    RC rc2 = new_record.copy_data(record.data(), record.len());
    if (rc2 != RC::SUCCESS) {
      LOG_ERROR("Failed to copy record data before update. rc=%d", rc2);
      continue;
    }
    new_record.set_rid(record.rid());
    // 使用 Record::set_field 修改字段（要求 new_record 拥有内存）
    rc2 = new_record.set_field(field->offset(), field->len(), values[0].data());
    if (rc2 != RC::SUCCESS) {
      LOG_ERROR("Failed to set field in new record. rc=%d", rc2);
      continue;
    }

    rc = table->update_record_with_trx(record, new_record, trx);
    if (rc == RC::SUCCESS) {
      update_count++;
    } else {
      LOG_WARN("update_record_with_trx failed. rc=%d", rc);
    }
  }
  delete scanner;
  if (update_count == 0) return RC::NOTFOUND;
  return RC::SUCCESS;
}
