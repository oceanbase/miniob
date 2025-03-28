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
// Created by Meiyi & Wangyunlai on 2021/5/12.
//

#include "common/lang/string.h"
#include "common/lang/algorithm.h"
#include "common/log/log.h"
#include "common/global_context.h"
#include "storage/table/table_meta.h"
#include "storage/trx/trx.h"
#include "json/json.h"

static const Json::StaticString FIELD_TABLE_ID("table_id");
static const Json::StaticString FIELD_TABLE_NAME("table_name");
static const Json::StaticString FIELD_STORAGE_FORMAT("storage_format");
static const Json::StaticString FIELD_STORAGE_ENGINE("storage_engine");
static const Json::StaticString FIELD_FIELDS("fields");
static const Json::StaticString FIELD_INDEXES("indexes");
static const Json::StaticString FIELD_PRIMARY_KEYS("primary_keys");

TableMeta::TableMeta(const TableMeta &other)
    : table_id_(other.table_id_),
      name_(other.name_),
      fields_(other.fields_),
      indexes_(other.indexes_),
      storage_format_(other.storage_format_),
      storage_engine_(other.storage_engine_),
      record_size_(other.record_size_)
{}

void TableMeta::swap(TableMeta &other) noexcept
{
  name_.swap(other.name_);
  fields_.swap(other.fields_);
  indexes_.swap(other.indexes_);
  std::swap(record_size_, other.record_size_);
}

RC TableMeta::init(int32_t table_id, const char *name, const vector<FieldMeta> *trx_fields,
                   span<const AttrInfoSqlNode> attributes, const vector<string> &primary_keys, StorageFormat storage_format,
                   StorageEngine storage_engine)
{
  if (common::is_blank(name)) {
    LOG_ERROR("Name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }

  if (attributes.size() == 0) {
    LOG_ERROR("Invalid argument. name=%s, field_num=%d", name, attributes.size());
    return RC::INVALID_ARGUMENT;
  }

  RC rc = RC::SUCCESS;

  int field_offset  = 0;
  int trx_field_num = 0;

  if (trx_fields != nullptr) {
    trx_fields_ = *trx_fields;

    fields_.resize(attributes.size() + trx_fields->size());
    for (size_t i = 0; i < trx_fields->size(); i++) {
      const FieldMeta &field_meta = (*trx_fields)[i];
      fields_[i] = FieldMeta(field_meta.name(), field_meta.type(), field_offset, field_meta.len(), false /*visible*/, field_meta.field_id());
      field_offset += field_meta.len();
    }

    trx_field_num = static_cast<int>(trx_fields->size());
  } else {
    fields_.resize(attributes.size());
  }

  for (size_t i = 0; i < attributes.size(); i++) {
    const AttrInfoSqlNode &attr_info = attributes[i];
    // `i` is the col_id of fields[i]
    rc = fields_[i + trx_field_num].init(
      attr_info.name.c_str(), attr_info.type, field_offset, attr_info.length, true /*visible*/, i);
    if (OB_FAIL(rc)) {
      LOG_ERROR("Failed to init field meta. table name=%s, field name: %s", name, attr_info.name.c_str());
      return rc;
    }

    field_offset += attr_info.length;
  }

  primary_keys_ = primary_keys;
  record_size_ = field_offset;

  table_id_ = table_id;
  name_     = name;
  storage_format_ = storage_format;
  storage_engine_ = storage_engine;
  LOG_INFO("Sussessfully initialized table meta. table id=%d, name=%s", table_id, name);
  return RC::SUCCESS;
}

RC TableMeta::add_index(const IndexMeta &index)
{
  indexes_.push_back(index);
  return RC::SUCCESS;
}

const char *TableMeta::name() const { return name_.c_str(); }

const FieldMeta *TableMeta::trx_field() const { return &fields_[0]; }

span<const FieldMeta> TableMeta::trx_fields() const
{
  return span<const FieldMeta>(fields_.data(), sys_field_num());
}

const FieldMeta *TableMeta::field(int index) const { return &fields_[index]; }
const FieldMeta *TableMeta::field(const char *name) const
{
  if (nullptr == name) {
    return nullptr;
  }
  for (const FieldMeta &field : fields_) {
    if (0 == strcmp(field.name(), name)) {
      return &field;
    }
  }
  return nullptr;
}

const FieldMeta *TableMeta::find_field_by_offset(int offset) const
{
  for (const FieldMeta &field : fields_) {
    if (field.offset() == offset) {
      return &field;
    }
  }
  return nullptr;
}
int TableMeta::field_num() const { return fields_.size(); }

int TableMeta::sys_field_num() const { return static_cast<int>(trx_fields_.size()); }

const IndexMeta *TableMeta::index(const char *name) const
{
  for (const IndexMeta &index : indexes_) {
    if (0 == strcmp(index.name(), name)) {
      return &index;
    }
  }
  return nullptr;
}

const IndexMeta *TableMeta::find_index_by_field(const char *field) const
{
  for (const IndexMeta &index : indexes_) {
    if (0 == strcmp(index.field(), field)) {
      return &index;
    }
  }
  return nullptr;
}

const IndexMeta *TableMeta::index(int i) const { return &indexes_[i]; }

int TableMeta::index_num() const { return indexes_.size(); }

int TableMeta::record_size() const { return record_size_; }

int TableMeta::serialize(ostream &ss) const
{
  Json::Value table_value;
  table_value[FIELD_TABLE_ID]   = table_id_;
  table_value[FIELD_TABLE_NAME] = name_;
  table_value[FIELD_STORAGE_FORMAT] = static_cast<int>(storage_format_);
  table_value[FIELD_STORAGE_ENGINE] = static_cast<int>(storage_engine_);

  Json::Value fields_value;
  for (const FieldMeta &field : fields_) {
    Json::Value field_value;
    field.to_json(field_value);
    fields_value.append(std::move(field_value));
  }

  table_value[FIELD_FIELDS] = std::move(fields_value);

  Json::Value indexes_value;
  for (const auto &index : indexes_) {
    Json::Value index_value;
    index.to_json(index_value);
    indexes_value.append(std::move(index_value));
  }
  table_value[FIELD_INDEXES] = std::move(indexes_value);

  Json::Value primary_keys_value;
  for (const auto &field : primary_keys_) {
    primary_keys_value.append(field);
  }
  table_value[FIELD_PRIMARY_KEYS] = std::move(primary_keys_value);

  Json::StreamWriterBuilder builder;
  Json::StreamWriter       *writer = builder.newStreamWriter();

  streampos old_pos = ss.tellp();
  writer->write(table_value, &ss);
  int ret = (int)(ss.tellp() - old_pos);

  delete writer;
  return ret;
}

int TableMeta::deserialize(istream &is)
{
  Json::Value             table_value;
  Json::CharReaderBuilder builder;
  string             errors;

  streampos old_pos = is.tellg();
  if (!Json::parseFromStream(builder, is, &table_value, &errors)) {
    LOG_ERROR("Failed to deserialize table meta. error=%s", errors.c_str());
    return -1;
  }

  const Json::Value &table_id_value = table_value[FIELD_TABLE_ID];
  if (!table_id_value.isInt()) {
    LOG_ERROR("Invalid table id. json value=%s", table_id_value.toStyledString().c_str());
    return -1;
  }

  int32_t table_id = table_id_value.asInt();

  const Json::Value &table_name_value = table_value[FIELD_TABLE_NAME];
  if (!table_name_value.isString()) {
    LOG_ERROR("Invalid table name. json value=%s", table_name_value.toStyledString().c_str());
    return -1;
  }

  string table_name = table_name_value.asString();

  const Json::Value &fields_value = table_value[FIELD_FIELDS];
  if (!fields_value.isArray() || fields_value.size() <= 0) {
    LOG_ERROR("Invalid table meta. fields is not array, json value=%s", fields_value.toStyledString().c_str());
    return -1;
  }

  const Json::Value &storage_format_value = table_value[FIELD_STORAGE_FORMAT];
  if (!storage_format_value.isInt()) {
    LOG_ERROR("Invalid storage format. json value=%s", storage_format_value.toStyledString().c_str());
    return -1;
  }

  int32_t storage_format = storage_format_value.asInt();

  const Json::Value &storage_engine_value = table_value[FIELD_STORAGE_ENGINE];
  if (!storage_engine_value.isInt()) {
    LOG_ERROR("Invalid storage engine. json value=%s", storage_engine_value.toStyledString().c_str());
    return -1;
  }

  int32_t storage_engine = storage_engine_value.asInt();

  RC  rc        = RC::SUCCESS;
  int field_num = fields_value.size();

  vector<FieldMeta> fields(field_num);
  for (int i = 0; i < field_num; i++) {
    FieldMeta &field = fields[i];

    const Json::Value &field_value = fields_value[i];
    rc                             = FieldMeta::from_json(field_value, field);
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to deserialize table meta. table name =%s", table_name.c_str());
      return -1;
    }
  }

  auto comparator = [](const FieldMeta &f1, const FieldMeta &f2) { return f1.offset() < f2.offset(); };
  sort(fields.begin(), fields.end(), comparator);

  table_id_ = table_id;
  storage_format_ = static_cast<StorageFormat>(storage_format);
  storage_engine_ = static_cast<StorageEngine>(storage_engine);
  name_.swap(table_name);
  fields_.swap(fields);
  record_size_ = fields_.back().offset() + fields_.back().len() - fields_.begin()->offset();

  for (const FieldMeta &field_meta : fields_) {
    if (!field_meta.visible()) {
      trx_fields_.push_back(field_meta); // 字段加上trx标识更好
    }
  }

  const Json::Value &indexes_value = table_value[FIELD_INDEXES];
  if (!indexes_value.empty()) {
    if (!indexes_value.isArray()) {
      LOG_ERROR("Invalid table meta. indexes is not array, json value=%s", fields_value.toStyledString().c_str());
      return -1;
    }
    const int              index_num = indexes_value.size();
    vector<IndexMeta> indexes(index_num);
    for (int i = 0; i < index_num; i++) {
      IndexMeta &index = indexes[i];

      const Json::Value &index_value = indexes_value[i];
      rc                             = IndexMeta::from_json(*this, index_value, index);
      if (rc != RC::SUCCESS) {
        LOG_ERROR("Failed to deserialize table meta. table name=%s", table_name.c_str());
        return -1;
      }
    }
    indexes_.swap(indexes);
  }

  const Json::Value &primary_keys_value = table_value[FIELD_PRIMARY_KEYS];
  if (!primary_keys_value.empty()) {
    if (!primary_keys_value.isArray()) {
      LOG_ERROR("Invalid table meta. primary keys is not array, json value=%s", fields_value.toStyledString().c_str());
      return -1;
    }
    const int              primary_key_num = primary_keys_value.size();
    vector<string> primary_keys(primary_key_num);
    for (int i = 0; i < primary_key_num; i++) {
      const Json::Value &field_name_value = primary_keys_value[i];
      if (!field_name_value.isString()) {
        LOG_ERROR("Invalid table meta. primary key name is not string, json value=%s",
                  field_name_value.toStyledString().c_str());
        return -1;
      }
      string field_name = field_name_value.asString();
      primary_keys.push_back(field_name);
    }
    primary_keys_.swap(primary_keys);
  }

  return (int)(is.tellg() - old_pos);
}

int TableMeta::get_serial_size() const { return -1; }

void TableMeta::to_string(string &output) const {}

void TableMeta::desc(ostream &os) const
{
  os << name_ << '(' << endl;
  for (const auto &field : fields_) {
    os << '\t';
    field.desc(os);
    os << endl;
  }

  for (const auto &index : indexes_) {
    os << '\t';
    index.desc(os);
    os << endl;
  }
  os << ')' << endl;
}
