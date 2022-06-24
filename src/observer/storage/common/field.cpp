#include "storage/common/field.h"
#include "common/log/log.h"

void Field::to_string(std::ostream &os) const
{
  switch (meta_->type()) {
  case INTS: {
    os << *(int *)data_;
  } break;
  case FLOATS: {
    os << *(float *)data_;
  } break;
  case CHARS: {
    for (int i = 0; i < 4; i++) { // the max length of CHARS is 4
      if (data_[i] == '\0') {
	break;
      }
      os << data_[i];
    }
  } break;
  default: {
    LOG_WARN("unsupported attr type: %d", meta_->type());
  } break;
  }
}
