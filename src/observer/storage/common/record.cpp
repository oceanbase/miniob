#include "storage/common/record.h"
#include "storage/common/field.h"
#include "common/log/log.h"
#include "rc.h"

RC Record::field_at(int index, Field &field) const
{
  if (index < 0 || index >= fields_->size()) {
    LOG_WARN("invalid argument. index=%d", index);
    return RC::INVALID_ARGUMENT;
  }

  const FieldMeta &field_meta = (*fields_)[index];
  field.set_type(field_meta.type());
  field.set_data(this->data_ + field_meta.offset());
  return RC::SUCCESS;
}

RC Record::set_field_value(const Value &value, int index)
{
  // TODO
  return RC::UNIMPLENMENT;
}

RC Record::set_field_values(const Value *values, int value_num, int start_index)
{
  // TODO
  return RC::UNIMPLENMENT;
}
