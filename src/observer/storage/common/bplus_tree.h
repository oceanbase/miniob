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
// Created by Longda on 2021/4/13.
//
#ifndef __OBSERVER_STORAGE_COMMON_INDEX_MANAGER_H_
#define __OBSERVER_STORAGE_COMMON_INDEX_MANAGER_H_

#include "record_manager.h"
#include "storage/default/disk_buffer_pool.h"
#include "sql/parser/parse_defs.h"

struct IndexFileHeader {
  int attr_length;
  int key_length;
  AttrType attr_type;
  PageNum root_page; // 初始时，root_page一定是1
  int node_num;
  int order;
};

struct IndexNode {
  int is_leaf;
  int key_num;
  PageNum parent;
  char *keys;
  RID *rids;
};

struct TreeNode {
  int key_num;
  char **keys;
  TreeNode *parent;
  TreeNode *sibling;
  TreeNode *first_child;
};

struct Tree {
  AttrType attr_type;
  int attr_length;
  int order;
  TreeNode *root;
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
  RC get_entry(const char *pkey, RID *rid);

  RC sync();
public:
  RC print();
  RC print_tree();
protected:
  RC find_leaf(const char *pkey, PageNum *leaf_page);
  RC insert_into_leaf(PageNum leaf_page, const char *pkey, const RID *rid);
  RC insert_into_leaf_after_split(PageNum leaf_page, const char *pkey, const RID *rid);
  RC insert_into_parent(PageNum parent_page, PageNum leaf_page, const char *pkey, PageNum right_page);
  RC insert_into_new_root(PageNum leaf_page, const char *pkey, PageNum right_page);
  RC insert_intern_node(PageNum parent_page, PageNum leaf_page, PageNum right_page, const char *pkey);
  RC insert_intern_node_after_split(PageNum intern_page, PageNum leaf_page, PageNum right_page, const char *pkey);

  RC delete_entry_from_node(PageNum node_page, const char *pkey);
  RC delete_entry_internal(PageNum page_num, const char *pkey);
  RC coalesce_node(PageNum leaf_page, PageNum right_page);
  RC redistribute_nodes(PageNum left_page, PageNum right_page);

  RC find_first_index_satisfied(CompOp comp_op, const char *pkey, PageNum *page_num, int *rididx);
  RC get_first_leaf_page(PageNum *leaf_page);

private:
  IndexNode *get_index_node(char *page_data) const;

private:
  DiskBufferPool  * disk_buffer_pool_ = nullptr;
  int               file_id_ = -1;
  bool              header_dirty_ = false;
  IndexFileHeader   file_header_;

private:
  friend class BplusTreeScanner;
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
  BplusTreeHandler   & index_handler_;
  bool opened_ = false;
  CompOp comp_op_ = NO_OP;                      // 用于比较的操作符
  const char *value_ = nullptr;		              // 与属性行比较的值
  int num_fixed_pages_ = -1;                    // 固定在缓冲区中的页，与指定的页面固定策略有关
  int pinned_page_count_ = 0;                   // 实际固定在缓冲区的页面数
  BPPageHandle page_handles_[BP_BUFFER_SIZE];   // 固定在缓冲区页面所对应的页面操作列表
  int next_index_of_page_handle_ = -1;          // 当前被扫描页面的操作索引
  int index_in_node_ = -1;                      // 当前B+ Tree页面上的key index
  PageNum next_page_num_ = -1;                  // 下一个将要被读入的页面号
};

#endif //__OBSERVER_STORAGE_COMMON_INDEX_MANAGER_H_