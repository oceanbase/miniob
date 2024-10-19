#include "common/log/log.h"
#include "common/type/boolean_type.h"
#include "common/type/attr_type.h"
#include "common/value.h"

RC BooleanType::to_string(const Value &val, string &result) const
{
  result = (val.value_.bool_value_)?"true":"false";
  return RC::SUCCESS;
}

int BooleanType::compare(const Value &left, const Value &right) const{
  return -1;
}

RC BooleanType::cast_to(const Value &val, AttrType type, Value &result) const {
  return RC::INTERNAL;
}

int BooleanType::cast_cost(AttrType type){
  return -1;
}
