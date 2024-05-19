#include "sql/operator/update_physical_operator.h"
#include "common/log/log.h"
#include "sql/stmt/update_stmt.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
#include<vector>
#include "src/observer/storage/record/record_manager.h"
#include "src/observer/storage/table/table.h"
#include "src/observer/storage/trx/vacuous_trx.h"

UpdatePhysicalOperator::UpdatePhysicalOperator(Table *table, Field field, Value value)
    :table_(table), field_(field), value_(value)
{}

RC UpdatePhysicalOperator::open(Trx *trx)
{
    if(children_.empty()) {
        return RC::SUCCESS;
    }

    std::unique_ptr<PhysicalOperator> &child = children_[0];
    RC                                 rc    = child->open(trx);
    if(rc != RC::SUCCESS) {
        LOG_WARN("failed to open child operator: %s", strrc(rc));
        return rc;
    }

    trx_ = trx;

    return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next()
{
    RC rc = RC::SUCCESS;
    if(children_.empty()){
        return RC::RECORD_EOF;
    }

    PhysicalOperator *child = children_[0].get();
    while(RC::SUCCESS == (rc = child->next())) {
        Tuple *tuple = child->current_tuple();
        if(nullptr == tuple) {
            LOG_WARN("failed to get current record: %s", strrc(rc));
            return rc;
        }

        //获取需要更新的record
        RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
        Record &record = row_tuple->record();

        //获取fieldmeta的偏移量及长度，确定更新的位置和范围
        const char *field_name = field_.field_name();
        const FieldMeta *field_meta = table_->table_meta().field(field_name);

        int offset_ = field_meta->offset();
        int len_ = field_meta->len();

        rc = trx_->update_record(table_, record, offset_, len_, value_);

        if(rc != RC::SUCCESS) {
            LOG_WARN("failed to delete record: %s", strrc(rc));
            return rc;
        }
    }

    return RC::RECORD_EOF;
    
}

RC UpdatePhysicalOperator::close()
{
    if(!children_.empty()) {
        children_[0]->close();
    }
    return RC::SUCCESS;
}