#pragma once

#include "sql/operator/operator.h"
#include "sql/expr/tuple.h"

class IndexScanOperator : public Operator
{
public: 
  IndexScanOperator(const Table *table, Index *index,
		    const TupleCell *left_cell, bool left_inclusive,
		    const TupleCell *right_cell, bool right_inclusive);

  virtual ~IndexScanOperator() = default;
  
  RC open() override;
  RC next() override;
  RC close() override;

  Tuple * current_tuple() override;
private:
  const Table *table_ = nullptr;
  Index *index_ = nullptr;
  IndexScanner *index_scanner_ = nullptr;
  RecordFileHandler *record_handler_ = nullptr;

  Record current_record_;
  RowTuple tuple_;

  TupleCell left_cell_;
  TupleCell right_cell_;
  bool left_inclusive_;
  bool right_inclusive_;
};
