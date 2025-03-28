/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "oblsm/table/ob_block_builder.h"
#include "oblsm/util/ob_coding.h"
#include "common/log/log.h"

namespace oceanbase {

void ObBlockBuilder::reset()
{
  offsets_.clear();
  data_.clear();
  // first_key_.clear();
}

RC ObBlockBuilder::add(const string_view &key, const string_view &value)
{
  RC rc = RC::SUCCESS;
  if (appro_size() + key.size() + value.size() + 2 * sizeof(uint32_t) > BLOCK_SIZE) {
    // TODO: support large kv pair.
    if (offsets_.empty()) {
      LOG_ERROR("block is empty, but kv pair is too large, key size: %lu, value size: %lu", key.size(), value.size());
      return RC::UNIMPLEMENTED;
    }
    LOG_TRACE("block is full, can't add more kv pair");
    rc = RC::FULL;
  } else {
    offsets_.push_back(data_.size());
    put_numeric<uint32_t>(&data_, key.size());
    data_.append(key.data(), key.size());
    put_numeric<uint32_t>(&data_, value.size());
    data_.append(value.data(), value.size());
  }
  return rc;
}

string ObBlockBuilder::last_key() const
{
  string_view last_kv(data_.data() + offsets_.back(), data_.size() - offsets_.back());
  uint32_t    key_length = get_numeric<uint32_t>(last_kv.data());
  return string(last_kv.data() + sizeof(uint32_t), key_length);
}

string_view ObBlockBuilder::finish()
{
  uint32_t data_size = data_.size();
  put_numeric<uint32_t>(&data_, offsets_.size());
  for (size_t i = 0; i < offsets_.size(); i++) {
    put_numeric<uint32_t>(&data_, offsets_[i]);
  }
  put_numeric<uint32_t>(&data_, data_size);
  return string_view(data_.data(), data_.size());
}

}  // namespace oceanbase
