# miniob - drop table 实现解析

> 此实现解析有往届选手提供。具体代码实现已经有所变更，因此仅供参考。

**代码部分主要添加在：**

drop table 与create table相反，要清理掉所有创建表和表相关联的资源，比如描述表的文件、数据文件以及索引等相关数据和文件。

sql流转到default_storge阶段的时候，在处理sql的函数中，新增一个drop_table的case。

drop table就是删除表，在`create table t`时，会新建一个t.table文件，同时为了存储数据也会新建一个t.data文件存储下来。同时创建索引的时候，也会创建记录索引数据的文件，在删除表时也要一起删除掉。

那么删除表，就需要**删除t.table文件、t.data文件和关联的索引文件**。

同时由于buffer pool的存在，在新建表和插入数据的时候，会写入buffer pool缓存。所以drop table，不仅需要删除文件，也需要**清空buffer pool** ，防止在数据没落盘的时候，再建立同名表，仍然可以查询到数据。

如果建立了索引，比如t_id on t(id)，那么也会新建一个t_id.index文件，也需要删除这个文件。

这些东西全部清空，那么就完成了drop table。

具体的代码实现如下：
在default_storage_stage.cpp 中的处理SQL语句的case中增加一个

```c++
  case SCF_DROP_TABLE: {
    const DropTable& drop_table = sql->sstr[sql->q_size-1].drop_table; // 拿到要drop 的表
    rc = handler_->drop_table(current_db,drop_table.relation_name); // 调用drop table接口，drop table要在handler中实现
    snprintf(response,sizeof(response),"%s\n", rc == RC::SUCCESS ? "SUCCESS" : "FAILURE"); // 返回结果，带不带换行符都可以
  }
break;
```

在default_handler.cpp文件中，实现handler的drop_table接口：

```c++
RC DefaultHandler::drop_table(const char *dbname, const char *relation_name) {
  Db *db = find_db(dbname);  // 这是原有的代码，用来查找对应的数据库，不过目前只有一个库
  if(db == nullptr) {
    return RC::SCHEMA_DB_NOT_OPENED;
  }
  return db->drop_table(relation_name); // 直接调用db的删掉接口
}
```

在db.cpp中，实现drop_table接口

```c++
RC Db::drop_table(const char* table_name)
{
    auto it = opened_tables_.find(table_name);
    if (it == opened_tables_.end())
    {
        return SCHEMA_TABLE_NOT_EXIST; // 找不到表，要返回错误，测试程序中也会校验这种场景
    }
    Table* table = it->second;
    RC rc = table->destroy(path_.c_str()); // 让表自己销毁资源
    if(rc != RC::SUCCESS) return rc;

    opened_tables_.erase(it); // 删除成功的话，从表list中将它删除
    delete table;
    return RC::SUCCESS;
}
```

table.cpp中清理文件和相关数据

```c++
RC Table::destroy(const char* dir) {
    RC rc = sync();//刷新所有脏页

    if(rc != RC::SUCCESS) return rc;

    std::string path = table_meta_file(dir, name());
    if(unlink(path.c_str()) != 0) {
        LOG_ERROR("Failed to remove meta file=%s, errno=%d", path.c_str(), errno);
        return RC::GENERIC_ERROR;
    }

    std::string data_file = std::string(dir) + "/" + name() + TABLE_DATA_SUFFIX;
    if(unlink(data_file.c_str()) != 0) { // 删除描述表元数据的文件
        LOG_ERROR("Failed to remove data file=%s, errno=%d", data_file.c_str(), errno);
        return RC::GENERIC_ERROR;
    }

    std::string text_data_file = std::string(dir) + "/" + name() + TABLE_TEXT_DATA_SUFFIX;
    if(unlink(text_data_file.c_str()) != 0) { // 删除表实现text字段的数据文件（后续实现了text case时需要考虑，最开始可以不考虑这个逻辑）
        LOG_ERROR("Failed to remove text data file=%s, errno=%d", text_data_file.c_str(), errno);
        return RC::GENERIC_ERROR;
    }

    const int index_num = table_meta_.index_num();
    for (int i = 0; i < index_num; i++) {  // 清理所有的索引相关文件数据与索引元数据
        ((BplusTreeIndex*)indexes_[i])->close();
        const IndexMeta* index_meta = table_meta_.index(i);
        std::string index_file = index_data_file(dir, name(), index_meta->name());
        if(unlink(index_file.c_str()) != 0) {
            LOG_ERROR("Failed to remove index file=%s, errno=%d", index_file.c_str(), errno);
            return RC::GENERIC_ERROR;
        }
    }
    return RC::SUCCESS;
}
```