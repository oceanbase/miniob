#pragma once

#include <iostream>
#include "storage/common/table.h"
#include "storage/common/field_meta.h"

class Field // TODO rename to Cell
{
public: 
  Field() = default;
  
  Field(FieldMeta *meta, char *data) : Field(nullptr, meta, data)
  {}
  Field(Table *table, FieldMeta *meta, char *data)
    : table_(table), attr_type_(meta->type()), data_(data)
  {}

  void set_table(Table *table) { this->table_ = table; }
  void set_type(AttrType type) { this->attr_type_ = type; }
  void set_data(char *data) { this->data_ = data; }
  void set_data(const char *data) { this->set_data(const_cast<char *>(data)); }

  void to_string(std::ostream &os) const;

  int compare(const Field &other) const;

private:
  Table *table_ = nullptr;
  AttrType attr_type_ = UNDEFINED;
  char *data_ = nullptr; // real data. no need to move to field_meta.offset
};
