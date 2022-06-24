#pragma once

#include <iostream>
#include "storage/common/table.h"
#include "storage/common/field_meta.h"

class Field
{
public: 
  Field() = default;
  
  Field(FieldMeta *meta, char *data) : Field(nullptr, meta, data)
  {}
  Field(Table *table, FieldMeta *meta, char *data)
    : table_(table), meta_(meta), data_(data)
  {}

  void set_table(Table *table) { this->table_ = table; }
  void set_meta(const FieldMeta *meta) { this->meta_ = meta; }
  void set_data(char *data) { this->data_ = data; }

  const FieldMeta *meta() const { return meta_; }
  void to_string(std::ostream &os) const;
private:
  Table *table_ = nullptr;
  const FieldMeta *meta_ = nullptr;
  char *data_ = nullptr; // real data. no need to move to field_meta.offset
};
