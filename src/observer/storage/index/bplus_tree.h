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

#pragma once

#include <string.h>

#include "common/lang/comparator.h"
#include "common/lang/memory.h"
#include "common/lang/sstream.h"
#include "common/lang/functional.h"
#include "common/log/log.h"
#include "sql/parser/parse_defs.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/record/record_manager.h"
#include "storage/index/latch_memo.h"
#include "storage/index/bplus_tree_log.h"

class BplusTreeHandler;
class BplusTreeMiniTransaction;

/**
 * @brief B+树的实现
 * @defgroup BPlusTree
 */

/**
 * @brief B+树的操作类型
 * @ingroup BPlusTree
 */
enum class BplusTreeOperationType
{
  READ,
  INSERT,
  DELETE,
};

/**
 * @brief 属性比较(BplusTree)
 * @ingroup BPlusTree
 */
class AttrComparator
{
public:
  void init(AttrType type, int length)
  {
    attr_type_   = type;
    attr_length_ = length;
  }

  int attr_length() const { return attr_length_; }

  int operator()(const char *v1, const char *v2) const
  {
    // TODO: optimized the comparison
    Value left;
    left.set_type(attr_type_);
    left.set_data(v1, attr_length_);
    Value right;
    right.set_type(attr_type_);
    right.set_data(v2, attr_length_);
    return DataType::type_instance(attr_type_)->compare(left, right);
  }

private:
  AttrType attr_type_;
  int      attr_length_;
};

/**
 * @brief 键值比较(BplusTree)
 * @details BplusTree的键值除了字段属性，还有RID，是为了避免属性值重复而增加的。
 * @ingroup BPlusTree
 */
class KeyComparator
{
public:
  void init(AttrType type, int length) { attr_comparator_.init(type, length); }

  const AttrComparator &attr_comparator() const { return attr_comparator_; }

  int operator()(const char *v1, const char *v2) const
  {
    int result = attr_comparator_(v1, v2);
    if (result != 0) {
      return result;
    }

    const RID *rid1 = (const RID *)(v1 + attr_comparator_.attr_length());
    const RID *rid2 = (const RID *)(v2 + attr_comparator_.attr_length());
    return RID::compare(rid1, rid2);
  }

private:
  AttrComparator attr_comparator_;
};

/**
 * @brief 属性打印,调试使用(BplusTree)
 * @ingroup BPlusTree
 */
class AttrPrinter
{
public:
  void init(AttrType type, int length)
  {
    attr_type_   = type;
    attr_length_ = length;
  }

  int attr_length() const { return attr_length_; }

  string operator()(const char *v) const
  {
    Value value(attr_type_, const_cast<char *>(v), attr_length_);
    return value.to_string();
  }

private:
  AttrType attr_type_;
  int      attr_length_;
};

/**
 * @brief 键值打印,调试使用(BplusTree)
 * @ingroup BPlusTree
 */
class KeyPrinter
{
public:
  void init(AttrType type, int length) { attr_printer_.init(type, length); }

  const AttrPrinter &attr_printer() const { return attr_printer_; }

  string operator()(const char *v) const
  {
    stringstream ss;
    ss << "{key:" << attr_printer_(v) << ",";

    const RID *rid = (const RID *)(v + attr_printer_.attr_length());
    ss << "rid:{" << rid->to_string() << "}}";
    return ss.str();
  }

private:
  AttrPrinter attr_printer_;
};

/**
 * @brief the meta information of bplus tree
 * @ingroup BPlusTree
 * @details this is the first page of bplus tree.
 * only one field can be supported, can you extend it to multi-fields?
 */
struct IndexFileHeader
{
  IndexFileHeader()
  {
    memset(this, 0, sizeof(IndexFileHeader));
    root_page = BP_INVALID_PAGE_NUM;
  }
  PageNum  root_page;          ///< 根节点在磁盘中的页号
  int32_t  internal_max_size;  ///< 内部节点最大的键值对数
  int32_t  leaf_max_size;      ///< 叶子节点最大的键值对数
  int32_t  attr_length;        ///< 键值的长度
  int32_t  key_length;         ///< attr length + sizeof(RID)
  AttrType attr_type;          ///< 键值的类型

  const string to_string() const
  {
    stringstream ss;

    ss << "attr_length:" << attr_length << ","
       << "key_length:" << key_length << ","
       << "attr_type:" << attr_type_to_string(attr_type) << ","
       << "root_page:" << root_page << ","
       << "internal_max_size:" << internal_max_size << ","
       << "leaf_max_size:" << leaf_max_size << ";";

    return ss.str();
  }
};

/**
 * @brief the common part of page describtion of bplus tree
 * @ingroup BPlusTree
 * @code
 * storage format:
 * | page type | item number | parent page id |
 * @endcode
 */
struct IndexNode
{
  static constexpr int HEADER_SIZE = 12;

  bool    is_leaf;  /// 当前是叶子节点还是内部节点
  int     key_num;  /// 当前页面上一共有多少个键值对
  PageNum parent;   /// 父节点页面编号
};

/**
 * @brief leaf page of bplus tree
 * @ingroup BPlusTree
 * @code
 * storage format:
 * | common header | prev page id | next page id |
 * | key0, rid0 | key1, rid1 | ... | keyn, ridn |
 * @endcode
 * the key is in format: the key value of record and rid.
 * so the key in leaf page must be unique.
 * the value is rid.
 * can you implenment a cluster index ?
 */
struct LeafIndexNode : public IndexNode
{
  static constexpr int HEADER_SIZE = IndexNode::HEADER_SIZE + 4;

  PageNum next_brother;
  /**
   * leaf can store order keys and rids at most
   */
  char array[0];
};

/**
 * @brief internal page of bplus tree
 * @ingroup BPlusTree
 * @code
 * storage format:
 * | common header |
 * | key(0),page_id(0) | key(1), page_id(1) | ... | key(n), page_id(n) |
 * @endcode
 * the first key is ignored(key0).
 * so it will waste space, can you fix this?
 */
struct InternalIndexNode : public IndexNode
{
  static constexpr int HEADER_SIZE = IndexNode::HEADER_SIZE;

  /**
   * internal node just store order -1 keys and order rids, the last rid is last right child.
   */
  char array[0];
};

/**
 * @brief IndexNode 仅作为数据在内存或磁盘中的表示
 * @ingroup BPlusTree
 * IndexNodeHandler 负责对IndexNode做各种操作。
 * 作为一个类来说，虚函数会影响“结构体”真实的内存布局，所以将数据存储与操作分开
 */
class IndexNodeHandler
{
public:
  IndexNodeHandler(BplusTreeMiniTransaction &mtr, const IndexFileHeader &header, Frame *frame);
  virtual ~IndexNodeHandler() = default;

  /// @brief 初始化一个新的页面
  /// @param leaf 是否叶子节点
  void init_empty(bool leaf);

  /// 是否叶子节点
  bool is_leaf() const;

  /// @brief 存储的键值大小
  virtual int key_size() const;
  /// @brief 存储的值的大小。内部节点和叶子节点是不一样的，但是这里返回的是叶子节点存储的大小
  virtual int value_size() const;
  /// @brief 存储的键值对的大小。值是指叶子节点中存放的数据
  virtual int item_size() const;

  void    increase_size(int n);
  int     size() const;
  int     max_size() const;
  int     min_size() const;
  RC      set_parent_page_num(PageNum page_num);
  PageNum parent_page_num() const;
  PageNum page_num() const;

  /**
   * @brief 判断对指定的操作，是否安全的
   * @details 安全是指在操作执行后，节点不需要调整，比如分裂、合并或重新分配
   * @param op 将要执行的操作
   * @param is_root_node 是否根节点
   */
  bool is_safe(BplusTreeOperationType op, bool is_root_node);

  /**
   * @brief 验证当前节点是否有问题
   */
  bool validate() const;

  Frame *frame() const { return frame_; }

  friend string to_string(const IndexNodeHandler &handler);

  RC recover_insert_items(int index, const char *items, int num);
  RC recover_remove_items(int index, int num);

protected:
  /**
   * @brief 获取指定元素的开始内存位置
   * @note 这并不是一个纯虚函数，是为了可以直接使用 IndexNodeHandler 类。
   * 但是使用这个类时，注意不能使用与这个函数相关的函数。
   */
  virtual char *__item_at(int index) const { return nullptr; }
  char         *__key_at(int index) const { return __item_at(index); }
  char         *__value_at(int index) const { return __item_at(index) + key_size(); };

protected:
  BplusTreeMiniTransaction &mtr_;
  const IndexFileHeader    &header_;
  Frame                    *frame_ = nullptr;
  IndexNode                *node_  = nullptr;
};

/**
 * @brief 叶子节点的操作
 * @ingroup BPlusTree
 */
class LeafIndexNodeHandler final : public IndexNodeHandler
{
public:
  LeafIndexNodeHandler(BplusTreeMiniTransaction &mtr, const IndexFileHeader &header, Frame *frame);
  virtual ~LeafIndexNodeHandler() = default;

  RC      init_empty();
  RC      set_next_page(PageNum page_num);
  PageNum next_page() const;

  char *key_at(int index);
  char *value_at(int index);

  /**
   * 查找指定key的插入位置(注意不是key本身)
   * 如果key已经存在，会设置found的值。
   */
  int lookup(const KeyComparator &comparator, const char *key, bool *found = nullptr) const;

  RC  insert(int index, const char *key, const char *value);
  RC  remove(int index);
  int remove(const char *key, const KeyComparator &comparator);
  RC  move_half_to(LeafIndexNodeHandler &other);
  RC  move_first_to_end(LeafIndexNodeHandler &other);
  RC  move_last_to_front(LeafIndexNodeHandler &other);
  /**
   * move all items to left page
   */
  RC move_to(LeafIndexNodeHandler &other);

  bool validate(const KeyComparator &comparator, DiskBufferPool *bp) const;

  friend string to_string(const LeafIndexNodeHandler &handler, const KeyPrinter &printer);

protected:
  char *__item_at(int index) const override;

  RC append(const char *items, int num);
  RC append(const char *item);
  RC preappend(const char *item);

private:
  LeafIndexNode *leaf_node_ = nullptr;
};

/**
 * @brief 内部节点的操作
 * @ingroup BPlusTree
 */
class InternalIndexNodeHandler final : public IndexNodeHandler
{
public:
  InternalIndexNodeHandler(BplusTreeMiniTransaction &mtr, const IndexFileHeader &header, Frame *frame);
  virtual ~InternalIndexNodeHandler() = default;

  RC init_empty();
  RC create_new_root(PageNum first_page_num, const char *key, PageNum page_num);

  RC      insert(const char *key, PageNum page_num, const KeyComparator &comparator);
  char   *key_at(int index);
  PageNum value_at(int index);

  /**
   * 返回指定子节点在当前节点中的索引
   */
  int  value_index(PageNum page_num);
  void set_key_at(int index, const char *key);
  void remove(int index);

  /**
   * 与Leaf节点不同，lookup返回指定key应该属于哪个子节点，返回这个子节点在当前节点中的索引
   * 如果想要返回插入位置，就提供 `insert_position` 参数
   * @param[in] comparator 用于键值比较的函数
   * @param[in] key 查找的键值
   * @param[out] found 如果是有效指针，将会返回当前是否存在指定的键值
   * @param[out] insert_position 如果是有效指针，将会返回可以插入指定键值的位置
   */
  int lookup(
      const KeyComparator &comparator, const char *key, bool *found = nullptr, int *insert_position = nullptr) const;

  /**
   * @brief 把当前节点的所有数据都迁移到另一个节点上
   *
   * @param other 数据迁移的目标节点
   */
  RC move_to(InternalIndexNodeHandler &other);
  RC move_first_to_end(InternalIndexNodeHandler &other);
  RC move_last_to_front(InternalIndexNodeHandler &other);
  RC move_half_to(InternalIndexNodeHandler &other);

  bool validate(const KeyComparator &comparator, DiskBufferPool *bp) const;

  friend string to_string(const InternalIndexNodeHandler &handler, const KeyPrinter &printer);

private:
  RC insert_items(int index, const char *items, int num);
  RC append(const char *items, int num);
  RC append(const char *item);
  RC preappend(const char *item);

private:
  char *__item_at(int index) const override;

  int value_size() const override;
  int item_size() const override;

private:
  InternalIndexNode *internal_node_ = nullptr;
};

/**
 * @brief B+树的实现
 * @ingroup BPlusTree
 */
class BplusTreeHandler
{
public:
  /**
   * @brief 创建一个B+树
   * @param log_handler 记录日志
   * @param bpm 缓冲池管理器
   * @param file_name 文件名
   * @param attr_type 属性类型
   * @param attr_length 属性长度
   * @param internal_max_size 内部节点最大大小
   * @param leaf_max_size 叶子节点最大大小
   */
  RC create(LogHandler &log_handler, BufferPoolManager &bpm, const char *file_name, AttrType attr_type, int attr_length,
      int internal_max_size = -1, int leaf_max_size = -1);
  RC create(LogHandler &log_handler, DiskBufferPool &buffer_pool, AttrType attr_type, int attr_length,
      int internal_max_size = -1, int leaf_max_size = -1);

  /**
   * @brief 打开一个B+树
   * @param log_handler 记录日志
   * @param bpm 缓冲池管理器
   * @param file_name 文件名
   */
  RC open(LogHandler &log_handler, BufferPoolManager &bpm, const char *file_name);
  RC open(LogHandler &log_handler, DiskBufferPool &buffer_pool);

  /**
   * 关闭句柄indexHandle对应的索引文件
   */
  RC close();

  /**
   * @brief 此函数向IndexHandle对应的索引中插入一个索引项。
   * @details 参数user_key指向要插入的属性值，参数rid标识该索引项对应的元组，
   * 即向索引中插入一个值为（user_key，rid）的键值对
   * @note 这里假设user_key的内存大小与attr_length 一致
   */
  RC insert_entry(const char *user_key, const RID *rid);

  /**
   * @brief 从IndexHandle句柄对应的索引中删除一个值为（user_key，rid）的索引项
   * @return RECORD_INVALID_KEY 指定值不存在
   * @note 这里假设user_key的内存大小与attr_length 一致
   */
  RC delete_entry(const char *user_key, const RID *rid);

  bool is_empty() const;

  /**
   * @brief 获取指定值的record
   * @param key_len user_key的长度
   * @param rid  返回值，记录记录所在的页面号和slot
   */
  RC get_entry(const char *user_key, int key_len, list<RID> &rids);

  RC sync();

  /**
   * Check whether current B+ tree is invalid or not.
   * @return true means current tree is valid, return false means current tree is invalid.
   * @note thread unsafe
   */
  bool validate_tree();

public:
  const IndexFileHeader &file_header() const { return file_header_; }
  DiskBufferPool        &buffer_pool() const { return *disk_buffer_pool_; }
  LogHandler            &log_handler() const { return *log_handler_; }

public:
  /**
   * @brief 恢复更新ROOT页面
   * @details 重做日志时调用的接口
   */
  RC recover_update_root_page(BplusTreeMiniTransaction &mtr, PageNum root_page_num);
  /**
   * @brief 恢复初始化头页面
   * @details 重做日志时调用的接口
   */
  RC recover_init_header_page(BplusTreeMiniTransaction &mtr, Frame *frame, const IndexFileHeader &header);

public:
  /**
   * 这些函数都是线程不安全的，不要在多线程的环境下调用
   */
  RC print_tree();
  RC print_leafs();

private:
  /**
   * 这些函数都是线程不安全的，不要在多线程的环境下调用
   */
  RC print_leaf(Frame *frame);
  RC print_internal_node_recursive(Frame *frame);

  bool validate_leaf_link(BplusTreeMiniTransaction &mtr);
  bool validate_node_recursive(BplusTreeMiniTransaction &mtr, Frame *frame);

protected:
  /**
   * @brief 查找叶子节点
   * @param op 当前想要执行的操作。操作类型不同会在查找的过程中加不同类型的锁
   * @param key 查找的键值
   * @param[out] frame 返回找到的叶子节点
   */
  RC find_leaf(BplusTreeMiniTransaction &mtr, BplusTreeOperationType op, const char *key, Frame *&frame);

  /**
   * @brief 找到最左边的叶子节点
   */
  RC left_most_page(BplusTreeMiniTransaction &mtr, Frame *&frame);

  /**
   * @brief 查找指定的叶子节点
   * @param op 当前想要执行的操作。操作类型不同会在查找的过程中加不同类型的锁
   * @param child_page_getter 用于获取子节点的函数
   * @param[out] frame 返回找到的叶子节点
   */
  RC find_leaf_internal(BplusTreeMiniTransaction &mtr, BplusTreeOperationType op,
      const function<PageNum(InternalIndexNodeHandler &)> &child_page_getter, Frame *&frame);

  /**
   * @brief 使用crabing protocol 获取页面
   */
  RC crabing_protocal_fetch_page(
      BplusTreeMiniTransaction &mtr, BplusTreeOperationType op, PageNum page_num, bool is_root_page, Frame *&frame);

  /**
   * @brief 从叶子节点中删除指定的键值对
   */
  RC delete_entry_internal(BplusTreeMiniTransaction &mtr, Frame *leaf_frame, const char *key);

  /**
   * @brief 拆分节点
   * @details 当节点中的键值对超过最大值时，需要拆分节点
   */
  template <typename IndexNodeHandlerType>
  RC split(BplusTreeMiniTransaction &mtr, Frame *frame, Frame *&new_frame);

  /**
   * @brief 合并或重新分配
   * @details 当节点中的键值对小于最小值时，需要合并或重新分配
   */
  template <typename IndexNodeHandlerType>
  RC coalesce_or_redistribute(BplusTreeMiniTransaction &mtr, Frame *frame);

  /**
   * @brief 合并两个相邻节点
   * @details 当节点中的键值对小于最小值并且相邻两个节点总和不超过最大节点个数时，需要合并两个相邻节点
   */
  template <typename IndexNodeHandlerType>
  RC coalesce(BplusTreeMiniTransaction &mtr, Frame *neighbor_frame, Frame *frame, Frame *parent_frame, int index);

  /**
   * @brief 重新分配两个相邻节点
   * @details 删除某个元素后，对应节点的元素个数比较少，并且与相邻节点不能合并，就将邻居节点的元素移动一些过来
   */
  template <typename IndexNodeHandlerType>
  RC redistribute(BplusTreeMiniTransaction &mtr, Frame *neighbor_frame, Frame *frame, Frame *parent_frame, int index);

  /**
   * @brief 在父节点插入一个元素
   */
  RC insert_entry_into_parent(BplusTreeMiniTransaction &mtr, Frame *frame, Frame *new_frame, const char *key);

  /**
   * @brief 在叶子节点插入一个元素
   */
  RC insert_entry_into_leaf_node(BplusTreeMiniTransaction &mtr, Frame *frame, const char *pkey, const RID *rid);

  /**
   * @brief 创建一个新的B+树
   */
  RC create_new_tree(BplusTreeMiniTransaction &mtr, const char *key, const RID *rid);

  /**
   * @brief 更新根节点的页号
   */
  void update_root_page_num_locked(BplusTreeMiniTransaction &mtr, PageNum root_page_num);

  /**
   * @brief 调整根节点
   */
  RC adjust_root(BplusTreeMiniTransaction &mtr, Frame *root_frame);

private:
  common::MemPoolItem::item_unique_ptr make_key(const char *user_key, const RID &rid);

protected:
  LogHandler     *log_handler_      = nullptr;  /// 日志处理器
  DiskBufferPool *disk_buffer_pool_ = nullptr;  /// 磁盘缓冲池
  bool            header_dirty_     = false;    /// 是否需要更新头页面
  IndexFileHeader file_header_;

  // 在调整根节点时，需要加上这个锁。
  // 这个锁可以使用递归读写锁，但是这里偷懒先不改
  common::SharedMutex root_lock_;

  KeyComparator key_comparator_;
  KeyPrinter    key_printer_;

  unique_ptr<common::MemPoolItem> mem_pool_item_;

private:
  friend class BplusTreeScanner;
  friend class BplusTreeTester;
};

/**
 * @brief B+树的扫描器
 * @ingroup BPlusTree
 */
class BplusTreeScanner
{
public:
  BplusTreeScanner(BplusTreeHandler &tree_handler);
  ~BplusTreeScanner();

  /**
   * @brief 扫描指定范围的数据
   * @param left_user_key 扫描范围的左边界，如果是null，则没有左边界
   * @param left_len left_user_key 的内存大小(只有在变长字段中才会关注)
   * @param left_inclusive 左边界的值是否包含在内
   * @param right_user_key 扫描范围的右边界。如果是null，则没有右边界
   * @param right_len right_user_key 的内存大小(只有在变长字段中才会关注)
   * @param right_inclusive 右边界的值是否包含在内
   * TODO 重构参数表示方法
   */
  RC open(const char *left_user_key, int left_len, bool left_inclusive, const char *right_user_key, int right_len,
      bool right_inclusive);

  /**
   * @brief 获取下一条记录
   *
   * @param rid 当前默认所有值都是RID类型。对B+树来说并不是一个好的抽象
   * @return RC RECORD_EOF 表示遍历完成
   * TODO 需要增加返回 key 的接口
   * @warning 不要在遍历时删除数据。删除数据会导致遍历器失效。
   * 当前默认的走索引删除的逻辑就是这样做的，所以删除逻辑有BUG。
   */
  RC next_entry(RID &rid);

  /**
   * @brief 关闭当前扫描器
   * @details 可以不调用，在析构函数时会自动执行
   */
  RC close();

private:
  /**
   * 如果key的类型是CHARS, 扩展或缩减user_key的大小刚好是schema中定义的大小
   */
  RC fix_user_key(const char *user_key, int key_len, bool want_greater, char **fixed_key, bool *should_inclusive);

  void fetch_item(RID &rid);

  /**
   * @brief 判断是否到了扫描的结束位置
   */
  bool touch_end();

private:
  bool                     inited_ = false;
  BplusTreeHandler        &tree_handler_;
  BplusTreeMiniTransaction mtr_;

  /// 使用左右叶子节点和位置来表示扫描的起始位置和终止位置
  /// 起始位置和终止位置都是有效的数据
  Frame *current_frame_ = nullptr;

  common::MemPoolItem::item_unique_ptr right_key_;
  int                                  iter_index_    = -1;
  bool                                 first_emitted_ = false;
};
