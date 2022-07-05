#pragma once

#include <iostream>
#include "storage/common/table.h"
#include "storage/common/field_meta.h"

class TupleCell
{
public: 
  TupleCell() = default;
  
  TupleCell(FieldMeta *meta, char *data)
    : TupleCell(meta->type(), data)
  {}
  TupleCell(AttrType attr_type, char *data)
    : attr_type_(attr_type), data_(data)
  {}

  void set_type(AttrType type) { this->attr_type_ = type; }
  void set_data(char *data) { this->data_ = data; }
  void set_data(const char *data) { this->set_data(const_cast<char *>(data)); }

  void to_string(std::ostream &os) const;

  int compare(const TupleCell &other) const;

private:
  AttrType attr_type_ = UNDEFINED;
  char *data_ = nullptr; // real data. no need to move to field_meta.offset
};
