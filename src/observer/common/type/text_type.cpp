#include "common/type/text_type.h"
#include "common/value.h"
#include "common/log/log.h"

TextType::TextType() { attr_type_ = AttrType::TEXTS; }

int TextType::compare(const Value &left, const Value &right) const { return CharType::compare(left, right); }

RC TextType::cast_to(const Value &val, AttrType type, Value &result) const
{
  return CharType::cast_to(val, type, result);
}

RC TextType::set_value_from_str(Value &val, const string &data) const
{
  val.set_string(data.c_str(), static_cast<int>(data.size()), AttrType::TEXTS);
  return RC::SUCCESS;
}

int TextType::cast_cost(AttrType type) { return CharType::cast_cost(type); }

RC TextType::to_string(const Value &val, string &result) const { return CharType::to_string(val, result); }