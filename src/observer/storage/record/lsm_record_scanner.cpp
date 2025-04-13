/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/record/lsm_record_scanner.h"
#include "storage/common/codec.h"

RC LsmRecordScanner::open_scan()
{
  RC rc = RC::SUCCESS;
  lsm_iter_ = oblsm_->new_iterator(ObLsmReadOptions());
  bytes encoded_key;
  rc = Codec::encode_without_rid(table_->table_id(), encoded_key);
  if (RC::SUCCESS != rc) {
    LOG_WARN("failed to encode table id");
    return rc;
  }
  lsm_iter_->seek(string_view((char *)encoded_key.data(), encoded_key.size()));
  tuple_.set_schema(table_, table_->table_meta().field_metas());
  return rc;
}


RC LsmRecordScanner::close_scan()
{
  delete lsm_iter_;
  lsm_iter_ = nullptr;
  return RC::SUCCESS;
}

RC LsmRecordScanner::next(Record &record)
{
  if (lsm_iter_->valid()) {
    // string_view lsm_key = lsm_iter_->key();
    string_view lsm_value = lsm_iter_->value();
    // record_.set_data_owner(lsm_value.data(), lsm_value.length());
    // // record_.set_rid();
    // tuple_.set_record(&record_);
    string_view lsm_key = lsm_iter_->key();
    int64_t table_id = 0;
    bytes lsm_key_bytes(lsm_key.begin(), lsm_key.end());
    Codec::decode(lsm_key_bytes, table_id);
    if (table_id != table_->table_id()) {
      return RC::RECORD_EOF;
    }

    record.copy_data((char *)lsm_value.data(), lsm_value.length());
    lsm_iter_->next();
    return RC::SUCCESS;

  } else {
    return RC::RECORD_EOF;
  }
  return RC::SUCCESS;
}