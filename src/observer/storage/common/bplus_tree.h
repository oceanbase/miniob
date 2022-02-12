/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
//
// Created by Xie Meiyi
// Rewritten by Longda & Wangyunlai
//
//
#ifndef __OBSERVER_STORAGE_COMMON_INDEX_MANAGER_H_
#define __OBSERVER_STORAGE_COMMON_INDEX_MANAGER_H_

#include <sstream>

#include "record_manager.h"
#include "storage/default/disk_buffer_pool.h"
#include "sql/parser/parse_defs.h"

#define EMPTY_RID_PAGE_NUM -1
#define EMPTY_RID_SLOT_NUM -1

struct IndexFileHeader {
  IndexFileHeader()
  {
    memset(this, 0, sizeof(IndexFileHeader));
  }
  int attr_length;
  int key_length;
  AttrType attr_type;
  PageNum root_page;
  int order;

  const std::string to_string()
  {
    std::stringstream ss;

    ss << "attr_length:" << attr_length << ","
       << "key_length:" << key_length << ","
       << "attr_type:" << attr_type << ","
       << "root_page:" << root_page << ","
       << "order:" << order << ";";

    return ss.str();
  }
};

#define RECORD_RESERVER_PAIR_NUM 2
struct IndexNode {
  bool is_leaf;
  int key_num;
  PageNum parent;
  PageNum prev_brother;  // valid when is_leaf = true
  PageNum next_brother;  // valid when is_leaf = true
  /**
   * leaf can store order keys and rids at most
   * internal node just store order -1 keys and order rids, the last rid is last rght child.
   */
  char *keys;
  /**
   * In the node which isn't leaf, the rids point to child's page,
   *                               rids[i] is keys[i]'s left child, rids[key_num] is the last right child.
   * In the node which is leaf, the rids point to record's rid.
   */
  RID *rids;

  void init_empty(IndexFileHeader &file_header)
  {
    is_leaf = true;
    key_num = 0;
    parent = -1;
    prev_brother = -1;
    next_brother = -1;
    keys = (char *)(this + 1);
    rids = (RID *)(keys + (file_header.order + RECORD_RESERVER_PAIR_NUM) * file_header.key_length);
  }

  std::string to_string(IndexFileHeader &file_header)
  {
    std::stringstream ss;

    ss << "is_leaf:" << is_leaf << ","
       << "key_num:" << key_num << ","
       << "parent:" << parent << ","
       << "prev_brother:" << prev_brother << ","
       << "next_brother:" << next_brother << ",";

    if (file_header.attr_type == INTS) {  // CHARS, INTS, FLOATS

      ss << "start_key:" << *(int *)(keys) << ","
         << "end_key:" << *(int *)(keys + (key_num - 1) * file_header.key_length) << ";";
    } else if (file_header.attr_type == FLOATS) {
      ss << "start_key:" << *(float *)(keys) << ","
         << "end_key:" << *(float *)(keys + (key_num - 1) * file_header.key_length) << ";";
    } else if (file_header.attr_type == CHARS) {
      char *temp = (char *)malloc(file_header.attr_length + 1);
      memset(temp, 0, file_header.attr_length + 1);
      memcpy(temp, keys, file_header.attr_length);
      ss << "start_key:" << temp << ",";
      memcpy(temp, keys + (key_num - 1) * file_header.key_length, file_header.attr_length);
      ss << "end_key:" << temp << ";";

      free(temp);
    } else {
      ss << "Unkown key range." << std::endl;
    }

    return ss.str();
  }
};

class BplusTreeHandler {
public:
  /**
   * 此函数创建一个名为fileName的索引。
   * attrType描述被索引属性的类型，attrLength描述被索引属性的长度
   */
  RC create(const char *file_name, AttrType attr_type, int attr_length);

  /**
   * 打开名为fileName的索引文件。
   * 如果方法调用成功，则indexHandle为指向被打开的索引句柄的指针。
   * 索引句柄用于在索引中插入或删除索引项，也可用于索引的扫描
   */
  RC open(const char *file_name);

  /**
   * 关闭句柄indexHandle对应的索引文件
   */
  RC close();

  /**
   * 此函数向IndexHandle对应的索引中插入一个索引项。
   * 参数pData指向要插入的属性值，参数rid标识该索引项对应的元组，
   * 即向索引中插入一个值为（*pData，rid）的键值对
   */
  RC insert_entry(const char *pkey, const RID *rid);

  /**
   * 从IndexHandle句柄对应的索引中删除一个值为（*pData，rid）的索引项
   * @return RECORD_INVALID_KEY 指定值不存在
   */
  RC delete_entry(const char *pkey, const RID *rid);

  /**
   * 获取指定值的record
   * @param rid  返回值，记录记录所在的页面号和slot
   */
  RC get_entry(const char *pkey, std::list<RID> &rids);

  RC sync();

  const int get_file_id()
  {
    return file_id_;
  }

  /**
   * Check whether current B+ tree is invalid or not.
   * return true means current tree is valid, return false means current tree is invalid.
   * @return
   */
  bool validate_tree();

public:
  RC print_tree();
  RC print_node(IndexNode *node, PageNum page_num);
  RC print_leafs();

  bool validate_node(IndexNode *node);
  bool validate_leaf_link();

protected:
  RC find_leaf(const char *pkey, PageNum *leaf_page);

  RC insert_into_parent(
      PageNum parent_page, BPPageHandle &left_page_handle, const char *pkey, BPPageHandle &right_page_handle);
  RC insert_intern_node(BPPageHandle &parent_page_handle, BPPageHandle &left_page_handle,
      BPPageHandle &right_page_handle, const char *pkey);
  RC split_leaf(BPPageHandle &leaf_page_handle);
  RC split_intern_node(BPPageHandle &parent_page_handle, const char *pkey);

  RC delete_entry_internal(PageNum page_num, const char *pkey);
  RC coalesce_node(BPPageHandle &parent_handle, BPPageHandle &left_handle, BPPageHandle &right_handle, int delete_index,
      bool check_change_leaf_key, int node_delete_index, const char *pkey);

  RC insert_into_new_root(BPPageHandle &left_page_handle, const char *pkey, BPPageHandle &right_page_handle);
  RC clean_root_after_delete(IndexNode *old_root);

  RC insert_entry_into_node(IndexNode *node, const char *pkey, const RID *rid, PageNum left_page);
  RC delete_entry_from_node(IndexNode *node, const char *pkey, int &node_delete_index);
  void delete_entry_from_node(IndexNode *node, const int delete_index);

  RC redistribute_nodes(BPPageHandle &parent_handle, BPPageHandle &left_handle, BPPageHandle &right_handle);
  void redistribute_nodes(
      IndexNode *left_node, IndexNode *right_node, PageNum left_page, PageNum right_page, char *new_key);
  void merge_nodes(IndexNode *left_node, IndexNode *right_node, PageNum left_page, char *parent_key);
  RC can_merge_with_other(BPPageHandle *page_handle, PageNum page_num, bool *can_merge);
  void split_node(
      IndexNode *left_node, IndexNode *right_node, PageNum left_page, PageNum right_page, char *new_parent_key);
  void copy_node(IndexNode *to, IndexNode *from);

  void get_entry_from_leaf(IndexNode *node, const char *pkey, std::list<RID> &rids, bool &continue_check);
  RC find_first_index_satisfied(CompOp comp_op, const char *pkey, PageNum *page_num, int *rididx);
  RC get_first_leaf_page(PageNum *leaf_page);

  IndexNode *get_index_node(char *page_data) const;
  void swith_root(BPPageHandle &new_root_page_handle, IndexNode *root, PageNum root_page);

  void change_children_parent(RID *rid, int rid_len, PageNum new_parent_page);
  RC get_parent_changed_index(
      BPPageHandle &parent_handle, IndexNode *&parent, IndexNode *node, PageNum page_num, int &changed_index);
  RC change_leaf_parent_key_insert(IndexNode *node, int changed_indx, PageNum page_num);
  RC change_leaf_parent_key_delete(IndexNode *leaf, int delete_indx, const char *old_first_key);
  RC change_insert_leaf_link(IndexNode *left, IndexNode *right, PageNum right_page);
  RC change_delete_leaf_link(IndexNode *left, IndexNode *right, PageNum right_page);

protected:
  DiskBufferPool *disk_buffer_pool_ = nullptr;
  int file_id_ = -1;
  bool header_dirty_ = false;
  IndexFileHeader file_header_;

  BPPageHandle root_page_handle_;
  IndexNode *root_node_ = nullptr;

  common::MemPoolItem *mem_pool_item_ = nullptr;

private:
  friend class BplusTreeScanner;
  friend class BplusTreeTester;
};

class BplusTreeScanner {
public:
  BplusTreeScanner(BplusTreeHandler &index_handler);

  /**
   * 用于在indexHandle对应的索引上初始化一个基于条件的扫描。
   * compOp和*value指定比较符和比较值，indexScan为初始化后的索引扫描结构指针
   * 没有带两个边界的范围扫描
   */
  RC open(CompOp comp_op, const char *value);

  /**
   * 用于继续索引扫描，获得下一个满足条件的索引项，
   * 并返回该索引项对应的记录的ID
   */
  RC next_entry(RID *rid);

  /**
   * 关闭一个索引扫描，释放相应的资源
   */
  RC close();

  /**
   * 获取由fileName指定的B+树索引内容，返回指向B+树的指针。
   * 此函数提供给测试程序调用，用于检查B+树索引内容的正确性
   */
  // RC getIndexTree(char *fileName, Tree *index);

private:
  RC get_next_idx_in_memory(RID *rid);
  RC find_idx_pages();
  bool satisfy_condition(const char *key);

private:
  BplusTreeHandler &index_handler_;
  bool opened_ = false;
  CompOp comp_op_ = NO_OP;                     // 用于比较的操作符
  const char *value_ = nullptr;                // 与属性行比较的值
  int num_fixed_pages_ = -1;                   // 固定在缓冲区中的页，与指定的页面固定策略有关
  int pinned_page_count_ = 0;                  // 实际固定在缓冲区的页面数
  BPPageHandle page_handles_[BP_BUFFER_SIZE];  // 固定在缓冲区页面所对应的页面操作列表
  int next_index_of_page_handle_ = -1;         // 当前被扫描页面的操作索引
  int index_in_node_ = -1;                     // 当前B+ Tree页面上的key index
  PageNum next_page_num_ = -1;                 // 下一个将要被读入的页面号
};

class BplusTreeTester {
public:
  BplusTreeTester(BplusTreeHandler &index_handler) : index_handler_(index_handler)
  {}
  ~BplusTreeTester() = default;

  void set_order(int order)
  {
    if (order >= 2 && order % 2 == 0) {
      index_handler_.file_header_.order = order;
      LOG_INFO("Successfully set index %d's order as %d", index_handler_.file_id_, order);
    } else {
      LOG_INFO("Invalid input order argument %d", order);
    }
  }

  const int get_oder()
  {
    return index_handler_.file_header_.order;
  }

protected:
  BplusTreeHandler &index_handler_;
};

#endif  //__OBSERVER_STORAGE_COMMON_INDEX_MANAGER_H_