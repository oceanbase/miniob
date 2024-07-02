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
// Created by Xie Meiyi
// Rewritten by Longda & Wangyunlai
//

#include <span>

#include "storage/index/bplus_tree.h"
#include "common/lang/lower_bound.h"
#include "common/log/log.h"
#include "common/global_context.h"
#include "sql/parser/parse_defs.h"
#include "storage/buffer/disk_buffer_pool.h"

using namespace common;

/**
 * @brief B+树的第一个页面存放的位置
 * @details B+树数据放到Buffer Pool中，Buffer Pool把文件按照固定大小的页面拆分。
 * 第一个页面存放元数据。
 */
#define FIRST_INDEX_PAGE 1

int calc_internal_page_capacity(int attr_length)
{
  int item_size = attr_length + sizeof(RID) + sizeof(PageNum);
  int capacity  = ((int)BP_PAGE_DATA_SIZE - InternalIndexNode::HEADER_SIZE) / item_size;
  return capacity;
}

int calc_leaf_page_capacity(int attr_length)
{
  int item_size = attr_length + sizeof(RID) + sizeof(RID);
  int capacity  = ((int)BP_PAGE_DATA_SIZE - LeafIndexNode::HEADER_SIZE) / item_size;
  return capacity;
}

/////////////////////////////////////////////////////////////////////////////////
IndexNodeHandler::IndexNodeHandler(BplusTreeMiniTransaction &mtr, const IndexFileHeader &header, Frame *frame)
    : mtr_(mtr), header_(header), frame_(frame), node_((IndexNode *)frame->data())
{}

bool IndexNodeHandler::is_leaf() const { return node_->is_leaf; }
void IndexNodeHandler::init_empty(bool leaf)
{
  node_->is_leaf = leaf;
  node_->key_num = 0;
  node_->parent  = BP_INVALID_PAGE_NUM;
}
PageNum IndexNodeHandler::page_num() const { return frame_->page_num(); }

int IndexNodeHandler::key_size() const { return header_.key_length; }

int IndexNodeHandler::value_size() const
{
  // return header_.value_size;
  return sizeof(RID);
}

int IndexNodeHandler::item_size() const { return key_size() + value_size(); }

int IndexNodeHandler::size() const { return node_->key_num; }

int IndexNodeHandler::max_size() const { return is_leaf() ? header_.leaf_max_size : header_.internal_max_size; }

int IndexNodeHandler::min_size() const
{
  const int max = this->max_size();
  return max - max / 2;
}

void IndexNodeHandler::increase_size(int n) 
{
  node_->key_num += n; 
}

PageNum IndexNodeHandler::parent_page_num() const { return node_->parent; }

RC IndexNodeHandler::set_parent_page_num(PageNum page_num) 
{ 
  RC rc = mtr_.logger().set_parent_page(*this, page_num, this->node_->parent);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log set parent page. rc=%s", strrc(rc));
    return rc;
  }
  this->node_->parent = page_num;
  return rc;
}

/**
 * 检查一个节点经过插入或删除操作后是否需要分裂或合并操作
 * @return true 需要分裂或合并；
 *         false 不需要分裂或合并
 */
bool IndexNodeHandler::is_safe(BplusTreeOperationType op, bool is_root_node)
{
  switch (op) {
    case BplusTreeOperationType::READ: {
      return true;
    } break;
    case BplusTreeOperationType::INSERT: {
      return size() < max_size();
    } break;
    case BplusTreeOperationType::DELETE: {
      if (is_root_node) {  // 参考adjust_root
        if (node_->is_leaf) {
          return size() > 1;  // 根节点如果空的话，就需要删除整棵树
        }
        // not leaf
        // 根节点还有子节点，但是如果删除一个子节点后，只剩一个子节点，就要把自己删除，把唯一的子节点变更为根节点
        return size() > 2;
      }
      return size() > min_size();
    } break;
    default: {
      // do nothing
    } break;
  }

  ASSERT(false, "invalid operation type: %d", static_cast<int>(op));
  return false;
}

string to_string(const IndexNodeHandler &handler)
{
  stringstream ss;

  ss << "PageNum:" << handler.page_num() << ",is_leaf:" << handler.is_leaf() << ","
     << "key_num:" << handler.size() << ","
     << "parent:" << handler.parent_page_num() << ",";

  return ss.str();
}

bool IndexNodeHandler::validate() const
{
  if (parent_page_num() == BP_INVALID_PAGE_NUM) {
    // this is a root page
    if (size() < 1) {
      LOG_WARN("root page has no item");
      return false;
    }

    if (!is_leaf() && size() < 2) {
      LOG_WARN("root page internal node has less than 2 child. size=%d", size());
      return false;
    }
  }
  return true;
}

RC IndexNodeHandler::recover_insert_items(int index, const char *items, int num)
{
  const int item_size = this->item_size();
  if (index < size()) {
    memmove(__item_at(index + num), __item_at(index), (static_cast<size_t>(size()) - index) * item_size);
  }

  memcpy(__item_at(index), items, static_cast<size_t>(num) * item_size);
  increase_size(num);
  return RC::SUCCESS;
}

RC IndexNodeHandler::recover_remove_items(int index, int num)
{
  const int item_size = this->item_size();
  if (index < size() - num) {
    memmove(__item_at(index), __item_at(index + num), (static_cast<size_t>(size()) - index - num) * item_size);
  }

  increase_size(-num);
  return RC::SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
LeafIndexNodeHandler::LeafIndexNodeHandler(BplusTreeMiniTransaction &mtr, const IndexFileHeader &header, Frame *frame)
    : IndexNodeHandler(mtr, header, frame), leaf_node_((LeafIndexNode *)frame->data())
{}

RC LeafIndexNodeHandler::init_empty()
{
  RC rc = mtr_.logger().leaf_init_empty(*this);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log init empty leaf node. rc=%s", strrc(rc));
    return rc;
  }
  IndexNodeHandler::init_empty(true/*leaf*/);
  leaf_node_->next_brother = BP_INVALID_PAGE_NUM;
  return RC::SUCCESS;
}

RC LeafIndexNodeHandler::set_next_page(PageNum page_num) 
{ 
  RC rc = mtr_.logger().leaf_set_next_page(*this, page_num, leaf_node_->next_brother);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log set next page. rc=%s", strrc(rc));
    return rc;
  }

  leaf_node_->next_brother = page_num; 
  return RC::SUCCESS;
}

PageNum LeafIndexNodeHandler::next_page() const { return leaf_node_->next_brother; }

char *LeafIndexNodeHandler::key_at(int index)
{
  assert(index >= 0 && index < size());
  return __key_at(index);
}

char *LeafIndexNodeHandler::value_at(int index)
{
  assert(index >= 0 && index < size());
  return __value_at(index);
}

int LeafIndexNodeHandler::lookup(const KeyComparator &comparator, const char *key, bool *found /* = nullptr */) const
{
  const int                    size = this->size();
  common::BinaryIterator<char> iter_begin(item_size(), __key_at(0));
  common::BinaryIterator<char> iter_end(item_size(), __key_at(size));
  common::BinaryIterator<char> iter = lower_bound(iter_begin, iter_end, key, comparator, found);
  return iter - iter_begin;
}

RC LeafIndexNodeHandler::insert(int index, const char *key, const char *value)
{
  vector<char> item(key_size() + value_size());
  memcpy(item.data(), key, key_size());
  memcpy(item.data() + key_size(), value, value_size());

  RC rc = mtr_.logger().node_insert_items(*this, index, item, 1);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log insert item. rc=%s", strrc(rc));
    return rc;
  }

  recover_insert_items(index, item.data(), 1);
  return RC::SUCCESS;
}

RC LeafIndexNodeHandler::remove(int index)
{
  assert(index >= 0 && index < size());

  RC rc = mtr_.logger().node_remove_items(*this, index, span<const char>(__item_at(index), item_size()), 1);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log remove item. rc=%s", strrc(rc));
    return rc;
  }

  recover_remove_items(index, 1);
  return RC::SUCCESS;
}

int LeafIndexNodeHandler::remove(const char *key, const KeyComparator &comparator)
{
  bool found = false;
  int  index = lookup(comparator, key, &found);
  if (found) {
    this->remove(index);
    return 1;
  }
  return 0;
}

RC LeafIndexNodeHandler::move_half_to(LeafIndexNodeHandler &other)
{
  const int size       = this->size();
  const int move_index = size / 2;
  const int move_item_num = size - move_index;

  other.append(__item_at(move_index), move_item_num);

  RC rc = mtr_.logger().node_remove_items(*this, move_index, span<const char>(__item_at(move_index), move_item_num * item_size()), move_item_num);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log shrink leaf node. rc=%s", strrc(rc));
    return rc;
  }

  recover_remove_items(move_index, move_item_num);
  return RC::SUCCESS;
}
RC LeafIndexNodeHandler::move_first_to_end(LeafIndexNodeHandler &other)
{
  other.append(__item_at(0));

  return this->remove(0);
}

RC LeafIndexNodeHandler::move_last_to_front(LeafIndexNodeHandler &other)
{
  other.preappend(__item_at(size() - 1));

  this->remove(size() - 1);
  return RC::SUCCESS;
}
/**
 * move all items to left page
 */
RC LeafIndexNodeHandler::move_to(LeafIndexNodeHandler &other)
{
  other.append(__item_at(0), this->size());
  other.set_next_page(this->next_page());

  RC rc = mtr_.logger().node_remove_items(*this, 0, span<const char>(__item_at(0), this->size() * item_size()), this->size());
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log shrink leaf node. rc=%s", strrc(rc));
  }
  this->increase_size(-this->size());

  return RC::SUCCESS;
}

// 复制一些数据到当前节点的最右边
RC LeafIndexNodeHandler::append(const char *items, int num)
{
  RC rc = mtr_.logger().node_insert_items(*this, size(), span<const char>(items, num * item_size()), num);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log append items. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  return recover_insert_items(size(), items, num);
}

RC LeafIndexNodeHandler::append(const char *item)
{
  return append(item, 1);
}

RC LeafIndexNodeHandler::preappend(const char *item)
{
  return insert(0, item, item + key_size());
}

char *LeafIndexNodeHandler::__item_at(int index) const { return leaf_node_->array + (index * item_size()); }

string to_string(const LeafIndexNodeHandler &handler, const KeyPrinter &printer)
{
  stringstream ss;
  ss << to_string((const IndexNodeHandler &)handler) << ",next page:" << handler.next_page();
  ss << ",values=[" << printer(handler.__key_at(0));
  for (int i = 1; i < handler.size(); i++) {
    ss << "," << printer(handler.__key_at(i));
  }
  ss << "]";
  return ss.str();
}

bool LeafIndexNodeHandler::validate(const KeyComparator &comparator, DiskBufferPool *bp) const
{
  bool result = IndexNodeHandler::validate();
  if (false == result) {
    return false;
  }

  const int node_size = size();
  for (int i = 1; i < node_size; i++) {
    if (comparator(__key_at(i - 1), __key_at(i)) >= 0) {
      LOG_WARN("page number = %d, invalid key order. id1=%d,id2=%d, this=%s",
               page_num(), i - 1, i, to_string(*this).c_str());
      return false;
    }
  }

  PageNum parent_page_num = this->parent_page_num();
  if (parent_page_num == BP_INVALID_PAGE_NUM) {
    return true;
  }

  Frame *parent_frame = nullptr;
  RC     rc = bp->get_this_page(parent_page_num, &parent_frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to fetch parent page. page num=%d, rc=%d:%s", parent_page_num, rc, strrc(rc));
    return false;
  }

  InternalIndexNodeHandler parent_node(mtr_, header_, parent_frame);
  int                      index_in_parent = parent_node.value_index(this->page_num());
  if (index_in_parent < 0) {
    LOG_WARN("invalid leaf node. cannot find index in parent. this page num=%d, parent page num=%d",
             this->page_num(), parent_page_num);
    bp->unpin_page(parent_frame);
    return false;
  }

  if (0 != index_in_parent) {
    int cmp_result = comparator(__key_at(0), parent_node.key_at(index_in_parent));
    if (cmp_result < 0) {
      LOG_WARN("invalid leaf node. first item should be greate than or equal to parent item. "
               "this page num=%d, parent page num=%d, index in parent=%d",
               this->page_num(), parent_node.page_num(), index_in_parent);
      bp->unpin_page(parent_frame);
      return false;
    }
  }

  if (index_in_parent < parent_node.size() - 1) {
    int cmp_result = comparator(__key_at(size() - 1), parent_node.key_at(index_in_parent + 1));
    if (cmp_result >= 0) {
      LOG_WARN("invalid leaf node. last item should be less than the item at the first after item in parent."
               "this page num=%d, parent page num=%d, parent item to compare=%d",
               this->page_num(), parent_node.page_num(), index_in_parent + 1);
      bp->unpin_page(parent_frame);
      return false;
    }
  }
  bp->unpin_page(parent_frame);
  return true;
}

/////////////////////////////////////////////////////////////////////////////////
InternalIndexNodeHandler::InternalIndexNodeHandler(BplusTreeMiniTransaction &mtr, const IndexFileHeader &header, Frame *frame)
    : IndexNodeHandler(mtr, header, frame), internal_node_((InternalIndexNode *)frame->data())
{}

string to_string(const InternalIndexNodeHandler &node, const KeyPrinter &printer)
{
  stringstream ss;
  ss << to_string((const IndexNodeHandler &)node);
  ss << ",children:["
     << "{key:" << printer(node.__key_at(0)) << ","
     << "value:" << *(PageNum *)node.__value_at(0) << "}";

  for (int i = 1; i < node.size(); i++) {
    ss << ",{key:" << printer(node.__key_at(i)) << ",value:" << *(PageNum *)node.__value_at(i) << "}";
  }
  ss << "]";
  return ss.str();
}

RC InternalIndexNodeHandler::init_empty() 
{
  RC rc = mtr_.logger().internal_init_empty(*this);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log init empty internal node. rc=%s", strrc(rc));
  }
  IndexNodeHandler::init_empty(false/*leaf*/);
  return RC::SUCCESS;
}
RC InternalIndexNodeHandler::create_new_root(PageNum first_page_num, const char *key, PageNum page_num)
{
  RC rc = mtr_.logger().internal_create_new_root(*this, first_page_num, span<const char>(key, key_size()), page_num);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log create new root. rc=%s", strrc(rc));
  }

  memset(__key_at(0), 0, key_size());
  memcpy(__value_at(0), &first_page_num, value_size());
  memcpy(__item_at(1), key, key_size());
  memcpy(__value_at(1), &page_num, value_size());
  increase_size(2);
  return RC::SUCCESS;
}

/**
 * @brief insert one entry
 * @details the entry to be inserted will never at the first slot.
 * the right child page after split will always have bigger keys.
 * @NOTE We don't need to set child's parent page num here. You can see the callers for details
 * It's also friendly to unittest that we don't set the child's parent page num.
 */
RC InternalIndexNodeHandler::insert(const char *key, PageNum page_num, const KeyComparator &comparator)
{
  int insert_position = -1;
  lookup(comparator, key, nullptr, &insert_position);
  vector<char> item(key_size() + sizeof(PageNum));
  memcpy(item.data(), key, key_size());
  memcpy(item.data() + key_size(), &page_num, sizeof(PageNum));
  RC rc = mtr_.logger().node_insert_items(*this, insert_position, span<const char>(item.data(), item.size()), 1);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log insert item. rc=%s", strrc(rc));
    return rc;
  }

  return recover_insert_items(insert_position, item.data(), 1);
}

/**
 * @brief move half of the items to the other node ends
 */
RC InternalIndexNodeHandler::move_half_to(InternalIndexNodeHandler &other)
{
  const int size       = this->size();
  const int move_index = size / 2;
  const int move_num   = size - move_index;
  RC        rc         = other.append(this->__item_at(move_index), size - move_index);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to copy item to new node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  mtr_.logger().node_remove_items(*this, move_index, span<const char>(__item_at(move_index), move_num * item_size()), move_num);
  increase_size(-(size - move_index));
  return rc;
}

/**
 * lookup the first item which key <= item
 * @return unlike the leafNode, the return value is not the insert position,
 * but only the index of child to find.
 */
int InternalIndexNodeHandler::lookup(const KeyComparator &comparator, const char *key, bool *found /* = nullptr */,
    int *insert_position /*= nullptr */) const
{
  const int size = this->size();
  if (size == 0) {
    if (insert_position) {
      *insert_position = 1;
    }
    if (found) {
      *found = false;
    }
    return 0;
  }

  common::BinaryIterator<char> iter_begin(item_size(), __key_at(1));
  common::BinaryIterator<char> iter_end(item_size(), __key_at(size));
  common::BinaryIterator<char> iter = lower_bound(iter_begin, iter_end, key, comparator, found);
  int                          ret  = static_cast<int>(iter - iter_begin) + 1;
  if (insert_position) {
    *insert_position = ret;
  }

  if (ret >= size || comparator(key, __key_at(ret)) < 0) {
    return ret - 1;
  }
  return ret;
}

char *InternalIndexNodeHandler::key_at(int index)
{
  assert(index >= 0 && index < size());
  return __key_at(index);
}

void InternalIndexNodeHandler::set_key_at(int index, const char *key)
{
  assert(index >= 0 && index < size());

  mtr_.logger().internal_update_key(*this, index, span<const char>(key, key_size()), span<const char>(__key_at(index), key_size()));
  memcpy(__key_at(index), key, key_size());
}

PageNum InternalIndexNodeHandler::value_at(int index)
{
  assert(index >= 0 && index < size());
  return *(PageNum *)__value_at(index);
}

int InternalIndexNodeHandler::value_index(PageNum page_num)
{
  for (int i = 0; i < size(); i++) {
    if (page_num == *(PageNum *)__value_at(i)) {
      return i;
    }
  }
  return -1;
}

void InternalIndexNodeHandler::remove(int index)
{
  assert(index >= 0 && index < size());

  BplusTreeLogger &logger = mtr_.logger();
  RC rc = logger.node_remove_items(*this, index, span<const char>(__item_at(index), item_size()), 1);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log remove item. rc=%s. node=%s", strrc(rc), to_string(*this).c_str());
  }

  recover_remove_items(index, 1);
}

RC InternalIndexNodeHandler::move_to(InternalIndexNodeHandler &other)
{
  RC rc = other.append(__item_at(0), size());
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to copy items to other node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  rc = mtr_.logger().node_remove_items(*this, 0, span<const char>(__item_at(0), size() * item_size()), size());
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log shrink internal node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }
  recover_remove_items(0, size());
  return RC::SUCCESS;
}

RC InternalIndexNodeHandler::move_first_to_end(InternalIndexNodeHandler &other)
{
  RC rc = other.append(__item_at(0));
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to append item to others.");
    return rc;
  }

  remove(0);
  return rc;
}

RC InternalIndexNodeHandler::move_last_to_front(InternalIndexNodeHandler &other)
{
  RC rc = other.preappend(__item_at(size() - 1));
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to preappend to others");
    return rc;
  }

  rc = mtr_.logger().node_remove_items(*this, size() - 1, span<const char>(__item_at(size() - 1), item_size()), 1);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log shrink internal node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }
  increase_size(-1);
  return rc;
}

RC InternalIndexNodeHandler::insert_items(int index, const char *items, int num)
{
  RC rc = mtr_.logger().node_insert_items(*this, index, span<const char>(items, item_size() * num), num);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to log insert item. rc=%s", strrc(rc));
    return rc;
  }

  recover_insert_items(index, items, num);

  LatchMemo &latch_memo = mtr_.latch_memo();
  PageNum this_page_num = this->page_num();
  Frame *frame = nullptr;

  // 设置所有页面的父页面为当前页面
  // 这里会访问大量的页面，可能会将他们从磁盘加载到内存中而占用大量的buffer pool页面
  for (int i = 0; i < num; i++) {
    const PageNum page_num = *(const PageNum *)((items + i * item_size()) + key_size());
    rc = latch_memo.get_page(page_num, frame);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to set child's page num. child page num:%d, this page num=%d, rc=%d:%s",
               page_num, this_page_num, rc, strrc(rc));
      return rc;
    }
    IndexNodeHandler child_node(mtr_, header_, frame);
    child_node.set_parent_page_num(this_page_num); // 这里虽然对页面做了修改，但是并没有加写锁，因为父页面加了锁
    frame->mark_dirty();
  }

  return RC::SUCCESS;
}

/**
 * copy items from other node to self's right
 */
RC InternalIndexNodeHandler::append(const char *items, int num)
{
  return insert_items(size(), items, num);
}

RC InternalIndexNodeHandler::append(const char *item) { return this->append(item, 1); }

RC InternalIndexNodeHandler::preappend(const char *item)
{
  return this->insert_items(0, item, 1);
}

char *InternalIndexNodeHandler::__item_at(int index) const { return internal_node_->array + (index * item_size()); }

int InternalIndexNodeHandler::value_size() const { return sizeof(PageNum); }

int InternalIndexNodeHandler::item_size() const { return key_size() + this->value_size(); }

bool InternalIndexNodeHandler::validate(const KeyComparator &comparator, DiskBufferPool *bp) const
{
  bool result = IndexNodeHandler::validate();
  if (false == result) {
    return false;
  }

  const int node_size = size();
  for (int i = 2; i < node_size; i++) {
    if (comparator(__key_at(i - 1), __key_at(i)) >= 0) {
      LOG_WARN("page number = %d, invalid key order. id1=%d,id2=%d, this=%s",
          page_num(), i - 1, i, to_string(*this).c_str());
      return false;
    }
  }

  for (int i = 0; result && i < node_size; i++) {
    PageNum page_num = *(PageNum *)__value_at(i);
    if (page_num < 0) {
      LOG_WARN("this page num=%d, got invalid child page. page num=%d", this->page_num(), page_num);
    } else {
      Frame *child_frame = nullptr;
      RC     rc = bp->get_this_page(page_num, &child_frame);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to fetch child page while validate internal page. page num=%d, rc=%d:%s", 
                 page_num, rc, strrc(rc));
      } else {
        IndexNodeHandler child_node(mtr_, header_, child_frame);
        if (child_node.parent_page_num() != this->page_num()) {
          LOG_WARN("child's parent page num is invalid. child page num=%d, parent page num=%d, this page num=%d",
              child_node.page_num(), child_node.parent_page_num(), this->page_num());
          result = false;
        }
        bp->unpin_page(child_frame);
      }
    }
  }

  if (!result) {
    return result;
  }

  const PageNum parent_page_num = this->parent_page_num();
  if (parent_page_num == BP_INVALID_PAGE_NUM) {
    return result;
  }

  Frame *parent_frame = nullptr;
  RC     rc = bp->get_this_page(parent_page_num, &parent_frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to fetch parent page. page num=%d, rc=%d:%s", parent_page_num, rc, strrc(rc));
    return false;
  }

  InternalIndexNodeHandler parent_node(mtr_, header_, parent_frame);

  int index_in_parent = parent_node.value_index(this->page_num());
  if (index_in_parent < 0) {
    LOG_WARN("invalid internal node. cannot find index in parent. this page num=%d, parent page num=%d",
             this->page_num(), parent_page_num);
    bp->unpin_page(parent_frame);
    return false;
  }

  if (0 != index_in_parent) {
    int cmp_result = comparator(__key_at(1), parent_node.key_at(index_in_parent));
    if (cmp_result < 0) {
      LOG_WARN("invalid internal node. the second item should be greate than or equal to parent item. "
               "this page num=%d, parent page num=%d, index in parent=%d",
               this->page_num(), parent_node.page_num(), index_in_parent);
      bp->unpin_page(parent_frame);
      return false;
    }
  }

  if (index_in_parent < parent_node.size() - 1) {
    int cmp_result = comparator(__key_at(size() - 1), parent_node.key_at(index_in_parent + 1));
    if (cmp_result >= 0) {
      LOG_WARN("invalid internal node. last item should be less than the item at the first after item in parent."
               "this page num=%d, parent page num=%d, parent item to compare=%d",
               this->page_num(), parent_node.page_num(), index_in_parent + 1);
      bp->unpin_page(parent_frame);
      return false;
    }
  }
  bp->unpin_page(parent_frame);

  return result;
}

/////////////////////////////////////////////////////////////////////////////////

RC BplusTreeHandler::sync()
{
  if (header_dirty_) {
    Frame *frame = nullptr;

    RC rc = disk_buffer_pool_->get_this_page(FIRST_INDEX_PAGE, &frame);
    if (OB_SUCC(rc) && frame != nullptr) {
      char *pdata = frame->data();
      memcpy(pdata, &file_header_, sizeof(file_header_));
      frame->mark_dirty();
      disk_buffer_pool_->unpin_page(frame);
      header_dirty_ = false;
    } else {
      LOG_WARN("failed to sync index header file. file_desc=%d, rc=%s", disk_buffer_pool_->file_desc(), strrc(rc));
      // TODO: ingore?
    }
  }
  return disk_buffer_pool_->flush_all_pages();
}

RC BplusTreeHandler::create(LogHandler &log_handler,
                            BufferPoolManager &bpm,
                            const char *file_name, 
                            AttrType attr_type, 
                            int attr_length, 
                            int internal_max_size /* = -1*/,
                            int leaf_max_size /* = -1 */)
{
  RC rc = bpm.create_file(file_name);
  if (OB_FAIL(rc)) {
    LOG_WARN("Failed to create file. file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    return rc;
  }
  LOG_INFO("Successfully create index file:%s", file_name);

  DiskBufferPool *bp = nullptr;

  rc = bpm.open_file(log_handler, file_name, bp);
  if (OB_FAIL(rc)) {
    LOG_WARN("Failed to open file. file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    return rc;
  }
  LOG_INFO("Successfully open index file %s.", file_name);

  rc = this->create(log_handler, *bp, attr_type, attr_length, internal_max_size, leaf_max_size);
  if (OB_FAIL(rc)) {
    bpm.close_file(file_name);
    return rc;
  }

  LOG_INFO("Successfully create index file %s.", file_name);
  return rc;
}

RC BplusTreeHandler::create(LogHandler &log_handler,
            DiskBufferPool &buffer_pool,
            AttrType attr_type,
            int attr_length,
            int internal_max_size /* = -1 */,
            int leaf_max_size /* = -1 */)
{
  if (internal_max_size < 0) {
    internal_max_size = calc_internal_page_capacity(attr_length);
  }
  if (leaf_max_size < 0) {
    leaf_max_size = calc_leaf_page_capacity(attr_length);
  }

  log_handler_      = &log_handler;
  disk_buffer_pool_ = &buffer_pool;

  RC rc = RC::SUCCESS;

  BplusTreeMiniTransaction mtr(*this, &rc);

  Frame *header_frame = nullptr;

  rc = mtr.latch_memo().allocate_page(header_frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to allocate header page for bplus tree. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  if (header_frame->page_num() != FIRST_INDEX_PAGE) {
    LOG_WARN("header page num should be %d but got %d. is it a new file",
             FIRST_INDEX_PAGE, header_frame->page_num());
    return RC::INTERNAL;
  }

  char            *pdata         = header_frame->data();
  IndexFileHeader *file_header   = (IndexFileHeader *)pdata;
  file_header->attr_length       = attr_length;
  file_header->key_length        = attr_length + sizeof(RID);
  file_header->attr_type         = attr_type;
  file_header->internal_max_size = internal_max_size;
  file_header->leaf_max_size     = leaf_max_size;
  file_header->root_page         = BP_INVALID_PAGE_NUM;

  // 取消记录日志的原因请参考下面的sync调用的地方。
  // mtr.logger().init_header_page(header_frame, *file_header);

  header_frame->mark_dirty();

  memcpy(&file_header_, pdata, sizeof(file_header_));
  header_dirty_ = false;

  mem_pool_item_ = make_unique<common::MemPoolItem>("b+tree");
  if (mem_pool_item_->init(file_header->key_length) < 0) {
    LOG_WARN("Failed to init memory pool for index");
    close();
    return RC::NOMEM;
  }

  key_comparator_.init(file_header->attr_type, file_header->attr_length);
  key_printer_.init(file_header->attr_type, file_header->attr_length);

  /*
  虽然我们针对B+树记录了WAL，但是我们记录的都是逻辑日志，并没有记录某个页面如何修改的物理日志。
  在做恢复时，必须先创建出来一个tree handler对象。但是如果元数据页面不正确的话，我们无法创建一个正确的tree handler对象。
  因此这里取消第一次元数据页面修改的WAL记录，而改用更简单的方式，直接将元数据页面刷到磁盘。
  */
  rc = this->sync();
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to sync index header. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  LOG_INFO("Successfully create index");
  return RC::SUCCESS;
}

RC BplusTreeHandler::open(LogHandler &log_handler, BufferPoolManager &bpm, const char *file_name)
{
  if (disk_buffer_pool_ != nullptr) {
    LOG_WARN("%s has been opened before index.open.", file_name);
    return RC::RECORD_OPENNED;
  }

  DiskBufferPool    *disk_buffer_pool = nullptr;

  RC rc = bpm.open_file(log_handler, file_name, disk_buffer_pool);
  if (OB_FAIL(rc)) {
    LOG_WARN("Failed to open file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    return rc;
  }

  rc = this->open(log_handler, *disk_buffer_pool);
  if (OB_SUCC(rc)) {
    LOG_INFO("open b+tree success. filename=%s", file_name);
  }
  return rc;
}

RC BplusTreeHandler::open(LogHandler &log_handler, DiskBufferPool &buffer_pool)
{
  if (disk_buffer_pool_ != nullptr) {
    LOG_WARN("b+tree has been opened before index.open.");
    return RC::RECORD_OPENNED;
  }

  RC rc = RC::SUCCESS;

  Frame *frame = nullptr;
  rc           = buffer_pool.get_this_page(FIRST_INDEX_PAGE, &frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("Failed to get first page, rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  char *pdata = frame->data();
  memcpy(&file_header_, pdata, sizeof(IndexFileHeader));
  header_dirty_     = false;
  disk_buffer_pool_ = &buffer_pool;
  log_handler_      = &log_handler;

  mem_pool_item_ = make_unique<common::MemPoolItem>("b+tree");
  if (mem_pool_item_->init(file_header_.key_length) < 0) {
    LOG_WARN("Failed to init memory pool for index");
    close();
    return RC::NOMEM;
  }

  // close old page_handle
  buffer_pool.unpin_page(frame);

  key_comparator_.init(file_header_.attr_type, file_header_.attr_length);
  key_printer_.init(file_header_.attr_type, file_header_.attr_length);
  LOG_INFO("Successfully open index");
  return RC::SUCCESS;
}

RC BplusTreeHandler::close()
{
  if (disk_buffer_pool_ != nullptr) {
    disk_buffer_pool_->close_file();
  }

  disk_buffer_pool_ = nullptr;
  return RC::SUCCESS;
}

RC BplusTreeHandler::print_leaf(Frame *frame)
{
  BplusTreeMiniTransaction mtr(*this);
  LeafIndexNodeHandler leaf_node(mtr, file_header_, frame);
  LOG_INFO("leaf node: %s", to_string(leaf_node, key_printer_).c_str());
  disk_buffer_pool_->unpin_page(frame);
  return RC::SUCCESS;
}

RC BplusTreeHandler::print_internal_node_recursive(Frame *frame)
{
  RC rc = RC::SUCCESS;
  BplusTreeMiniTransaction mtr(*this);

  LOG_INFO("bplus tree. file header: %s", file_header_.to_string().c_str());
  InternalIndexNodeHandler internal_node(mtr, file_header_, frame);
  LOG_INFO("internal node: %s", to_string(internal_node, key_printer_).c_str());

  int node_size = internal_node.size();
  for (int i = 0; i < node_size; i++) {
    PageNum page_num = internal_node.value_at(i);
    Frame  *child_frame;
    rc = disk_buffer_pool_->get_this_page(page_num, &child_frame);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to fetch child page. page id=%d, rc=%d:%s", page_num, rc, strrc(rc));
      disk_buffer_pool_->unpin_page(frame);
      return rc;
    }

    IndexNodeHandler node(mtr, file_header_, child_frame);
    if (node.is_leaf()) {
      rc = print_leaf(child_frame);
    } else {
      rc = print_internal_node_recursive(child_frame);
    }
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to print node. page id=%d, rc=%d:%s", child_frame->page_num(), rc, strrc(rc));
      disk_buffer_pool_->unpin_page(frame);
      return rc;
    }
  }

  disk_buffer_pool_->unpin_page(frame);
  return RC::SUCCESS;
}

RC BplusTreeHandler::print_tree()
{
  if (disk_buffer_pool_ == nullptr) {
    LOG_WARN("Index hasn't been created or opened, fail to print");
    return RC::SUCCESS;
  }
  if (is_empty()) {
    LOG_INFO("tree is empty");
    return RC::SUCCESS;
  }

  Frame  *frame    = nullptr;
  PageNum page_num = file_header_.root_page;

  RC rc = disk_buffer_pool_->get_this_page(page_num, &frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to fetch page. page id=%d, rc=%d:%s", page_num, rc, strrc(rc));
    return rc;
  }

  BplusTreeMiniTransaction mtr(*this);

  IndexNodeHandler node(mtr, file_header_, frame);
  if (node.is_leaf()) {
    rc = print_leaf(frame);
  } else {
    rc = print_internal_node_recursive(frame);
  }
  return rc;
}

RC BplusTreeHandler::print_leafs()
{
  if (is_empty()) {
    LOG_INFO("empty tree");
    return RC::SUCCESS;
  }

  BplusTreeMiniTransaction mtr(*this);
  LatchMemo latch_memo = mtr.latch_memo();
  Frame    *frame = nullptr;

  RC rc = left_most_page(mtr, frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to get left most page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  while (frame->page_num() != BP_INVALID_PAGE_NUM) {
    LeafIndexNodeHandler leaf_node(mtr, file_header_, frame);
    LOG_INFO("leaf info: %s", to_string(leaf_node, key_printer_).c_str());

    PageNum next_page_num = leaf_node.next_page();
    latch_memo.release();

    if (next_page_num == BP_INVALID_PAGE_NUM) {
      break;
    }

    rc = latch_memo.get_page(next_page_num, frame);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to get next page. page id=%d, rc=%d:%s", next_page_num, rc, strrc(rc));
      return rc;
    }
  }
  return rc;
}

bool BplusTreeHandler::validate_node_recursive(BplusTreeMiniTransaction &mtr, Frame *frame)
{
  bool             result = true;
  IndexNodeHandler node(mtr, file_header_, frame);
  if (node.is_leaf()) {
    LeafIndexNodeHandler leaf_node(mtr, file_header_, frame);
    result = leaf_node.validate(key_comparator_, disk_buffer_pool_);
  } else {
    InternalIndexNodeHandler internal_node(mtr, file_header_, frame);
    result = internal_node.validate(key_comparator_, disk_buffer_pool_);
    for (int i = 0; result && i < internal_node.size(); i++) {
      PageNum page_num = internal_node.value_at(i);
      Frame  *child_frame = nullptr;
      RC      rc = mtr.latch_memo().get_page(page_num, child_frame);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to fetch child page.page id=%d, rc=%d:%s", page_num, rc, strrc(rc));
        result = false;
        break;
      }

      result = validate_node_recursive(mtr, child_frame);
    }
  }

  return result;
}

bool BplusTreeHandler::validate_leaf_link(BplusTreeMiniTransaction &mtr)
{
  if (is_empty()) {
    return true;
  }

  Frame *frame = nullptr;

  RC rc = left_most_page(mtr, frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to fetch left most page. rc=%d:%s", rc, strrc(rc));
    return false;
  }

  LeafIndexNodeHandler leaf_node(mtr, file_header_, frame);
  PageNum              next_page_num = leaf_node.next_page();

  MemPoolItem::item_unique_ptr prev_key = mem_pool_item_->alloc_unique_ptr();
  memcpy(prev_key.get(), leaf_node.key_at(leaf_node.size() - 1), file_header_.key_length);

  bool result = true;
  while (result && next_page_num != BP_INVALID_PAGE_NUM) {
    rc = mtr.latch_memo().get_page(next_page_num, frame);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to fetch next page. page num=%d, rc=%s", next_page_num, strrc(rc));
      return false;
    }

    LeafIndexNodeHandler leaf_node(mtr, file_header_, frame);
    if (key_comparator_((char *)prev_key.get(), leaf_node.key_at(0)) >= 0) {
      LOG_WARN("invalid page. current first key is not bigger than last");
      result = false;
    }

    next_page_num = leaf_node.next_page();
    memcpy(prev_key.get(), leaf_node.key_at(leaf_node.size() - 1), file_header_.key_length);
  }

  // can do more things
  return result;
}

bool BplusTreeHandler::validate_tree()
{
  if (is_empty()) {
    return true;
  }

  BplusTreeMiniTransaction mtr(*this);
  LatchMemo &latch_memo = mtr.latch_memo();
  Frame    *frame = nullptr;

  RC rc = latch_memo.get_page(file_header_.root_page, frame);  // 这里仅仅调试使用，不加root锁
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to fetch root page. page id=%d, rc=%d:%s", file_header_.root_page, rc, strrc(rc));
    return false;
  }

  if (!validate_node_recursive(mtr, frame) || !validate_leaf_link(mtr)) {
    LOG_WARN("Current B+ Tree is invalid");
    print_tree();
    return false;
  }

  LOG_INFO("great! current tree is valid");
  return true;
}

bool BplusTreeHandler::is_empty() const { return file_header_.root_page == BP_INVALID_PAGE_NUM; }

RC BplusTreeHandler::find_leaf(BplusTreeMiniTransaction &mtr, BplusTreeOperationType op, const char *key, Frame *&frame)
{
  auto child_page_getter = [this, key](InternalIndexNodeHandler &internal_node) {
    return internal_node.value_at(internal_node.lookup(key_comparator_, key));
  };
  return find_leaf_internal(mtr, op, child_page_getter, frame);
}

RC BplusTreeHandler::left_most_page(BplusTreeMiniTransaction &mtr, Frame *&frame)
{
  auto child_page_getter = [](InternalIndexNodeHandler &internal_node) { return internal_node.value_at(0); };
  return find_leaf_internal(mtr, BplusTreeOperationType::READ, child_page_getter, frame);
}

RC BplusTreeHandler::find_leaf_internal(BplusTreeMiniTransaction &mtr, BplusTreeOperationType op,
    const function<PageNum(InternalIndexNodeHandler &)> &child_page_getter, Frame *&frame)
{
  LatchMemo &latch_memo = mtr.latch_memo();

  // root locked
  if (op != BplusTreeOperationType::READ) {
    latch_memo.xlatch(&root_lock_);
  } else {
    latch_memo.slatch(&root_lock_);
  }

  if (is_empty()) {
    return RC::EMPTY;
  }

  RC rc = crabing_protocal_fetch_page(mtr, op, file_header_.root_page, true /* is_root_node */, frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to fetch root page. page id=%d, rc=%d:%s", file_header_.root_page, rc, strrc(rc));
    return rc;
  }

  IndexNode *node = (IndexNode *)frame->data();
  PageNum    next_page_id;
  for (; !node->is_leaf;) {
    InternalIndexNodeHandler internal_node(mtr, file_header_, frame);
    next_page_id = child_page_getter(internal_node);
    rc           = crabing_protocal_fetch_page(mtr, op, next_page_id, false /* is_root_node */, frame);
    if (OB_FAIL(rc)) {
      LOG_WARN("Failed to load page page_num:%d. rc=%s", next_page_id, strrc(rc));
      return rc;
    }

    node = (IndexNode *)frame->data();
  }
  return RC::SUCCESS;
}

RC BplusTreeHandler::crabing_protocal_fetch_page(
    BplusTreeMiniTransaction &mtr, BplusTreeOperationType op, PageNum page_num, bool is_root_node, Frame *&frame)
{
  LatchMemo &latch_memo = mtr.latch_memo();
  bool      readonly   = (op == BplusTreeOperationType::READ);
  const int memo_point = latch_memo.memo_point();

  RC rc = latch_memo.get_page(page_num, frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to get frame. pageNum=%d, rc=%s", page_num, strrc(rc));
    return rc;
  }

  LatchMemoType latch_type = readonly ? LatchMemoType::SHARED : LatchMemoType::EXCLUSIVE;
  mtr.latch_memo().latch(frame, latch_type);
  IndexNodeHandler index_node(mtr, file_header_, frame);
  if (index_node.is_safe(op, is_root_node)) {
    latch_memo.release_to(memo_point);  // 当前节点不会分裂或合并，可以将前面的锁都释放掉
  }
  return rc;
}

RC BplusTreeHandler::insert_entry_into_leaf_node(BplusTreeMiniTransaction &mtr, Frame *frame, const char *key, const RID *rid)
{
  LeafIndexNodeHandler leaf_node(mtr, file_header_, frame);
  bool                 exists          = false;  // 该数据是否已经存在指定的叶子节点中了
  int                  insert_position = leaf_node.lookup(key_comparator_, key, &exists);
  if (exists) {
    LOG_TRACE("entry exists");
    return RC::RECORD_DUPLICATE_KEY;
  }

  if (leaf_node.size() < leaf_node.max_size()) {
    leaf_node.insert(insert_position, key, (const char *)rid);
    frame->mark_dirty();
    // disk_buffer_pool_->unpin_page(frame); // unpin pages 由latch memo 来操作
    return RC::SUCCESS;
  }

  Frame *new_frame = nullptr;
  RC     rc        = split<LeafIndexNodeHandler>(mtr, frame, new_frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to split leaf node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  LeafIndexNodeHandler new_index_node(mtr, file_header_, new_frame);
  new_index_node.set_next_page(leaf_node.next_page());
  new_index_node.set_parent_page_num(leaf_node.parent_page_num());
  leaf_node.set_next_page(new_frame->page_num());

  if (insert_position < leaf_node.size()) {
    leaf_node.insert(insert_position, key, (const char *)rid);
  } else {
    new_index_node.insert(insert_position - leaf_node.size(), key, (const char *)rid);
  }

  return insert_entry_into_parent(mtr, frame, new_frame, new_index_node.key_at(0));
}

RC BplusTreeHandler::insert_entry_into_parent(BplusTreeMiniTransaction &mtr, Frame *frame, Frame *new_frame, const char *key)
{
  RC rc = RC::SUCCESS;

  IndexNodeHandler node_handler(mtr, file_header_, frame);
  IndexNodeHandler new_node_handler(mtr, file_header_, new_frame);
  PageNum          parent_page_num = node_handler.parent_page_num();

  if (parent_page_num == BP_INVALID_PAGE_NUM) {

    // create new root page
    Frame *root_frame = nullptr;
    rc = disk_buffer_pool_->allocate_page(&root_frame);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to allocate new root page. rc=%d:%s", rc, strrc(rc));
      return rc;
    }

    InternalIndexNodeHandler root_node(mtr, file_header_, root_frame);
    root_node.init_empty();
    root_node.create_new_root(frame->page_num(), key, new_frame->page_num());
    node_handler.set_parent_page_num(root_frame->page_num());
    new_node_handler.set_parent_page_num(root_frame->page_num());

    frame->mark_dirty();
    new_frame->mark_dirty();
    // disk_buffer_pool_->unpin_page(frame);
    // disk_buffer_pool_->unpin_page(new_frame);

    root_frame->write_latch();  // 在root页面更新之后，别人就可以访问到了，这时候就要加上锁
    update_root_page_num_locked(mtr, root_frame->page_num());
    root_frame->mark_dirty();
    root_frame->write_unlatch();
    disk_buffer_pool_->unpin_page(root_frame);

    return RC::SUCCESS;

  } else {

    Frame *parent_frame = nullptr;

    rc = mtr.latch_memo().get_page(parent_page_num, parent_frame);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to insert entry into leaf. rc=%d:%s", rc, strrc(rc));
      // we should do some things to recover
      return rc;
    }

    // 在第一次遍历这个页面时，我们已经拿到parent frame的write latch，所以这里不再去加锁
    InternalIndexNodeHandler parent_node(mtr, file_header_, parent_frame);

    /// 当前这个父节点还没有满，直接将新节点数据插进入就行了
    if (parent_node.size() < parent_node.max_size()) {
      parent_node.insert(key, new_frame->page_num(), key_comparator_);
      new_node_handler.set_parent_page_num(parent_page_num);

      frame->mark_dirty();
      new_frame->mark_dirty();
      parent_frame->mark_dirty();
      // disk_buffer_pool_->unpin_page(frame);
      // disk_buffer_pool_->unpin_page(new_frame);
      // disk_buffer_pool_->unpin_page(parent_frame);

    } else {

      // 当前父节点即将装满了，那只能再将父节点执行分裂操作
      Frame *new_parent_frame = nullptr;

      rc = split<InternalIndexNodeHandler>(mtr, parent_frame, new_parent_frame);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to split internal node. rc=%d:%s", rc, strrc(rc));
        // disk_buffer_pool_->unpin_page(frame);
        // disk_buffer_pool_->unpin_page(new_frame);
        // disk_buffer_pool_->unpin_page(parent_frame);
      } else {
        // insert into left or right ? decide by key compare result
        InternalIndexNodeHandler new_node(mtr, file_header_, new_parent_frame);
        if (key_comparator_(key, new_node.key_at(0)) > 0) {
          new_node.insert(key, new_frame->page_num(), key_comparator_);
          new_node_handler.set_parent_page_num(new_node.page_num());
        } else {
          parent_node.insert(key, new_frame->page_num(), key_comparator_);
          new_node_handler.set_parent_page_num(parent_node.page_num());
        }

        // disk_buffer_pool_->unpin_page(frame);
        // disk_buffer_pool_->unpin_page(new_frame);

        // 虽然这里是递归调用，但是通常B+ Tree 的层高比较低（3层已经可以容纳很多数据），所以没有栈溢出风险。
        // Q: 在查找叶子节点时，我们都会尝试将没必要的锁提前释放掉，在这里插入数据时，是在向上遍历节点，
        //    理论上来说，我们可以释放更低层级节点的锁，但是并没有这么做，为什么？
        rc = insert_entry_into_parent(mtr, parent_frame, new_parent_frame, new_node.key_at(0));
      }
    }
  }
  return rc;
}

/**
 * split one full node into two
 */
template <typename IndexNodeHandlerType>
RC BplusTreeHandler::split(BplusTreeMiniTransaction &mtr, Frame *frame, Frame *&new_frame)
{
  IndexNodeHandlerType old_node(mtr, file_header_, frame);

  // add a new node
  RC rc = mtr.latch_memo().allocate_page(new_frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("Failed to split index page due to failed to allocate page, rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  mtr.latch_memo().xlatch(new_frame);

  IndexNodeHandlerType new_node(mtr, file_header_, new_frame);
  new_node.init_empty();
  new_node.set_parent_page_num(old_node.parent_page_num());

  old_node.move_half_to(new_node);

  frame->mark_dirty();
  new_frame->mark_dirty();
  return RC::SUCCESS;
}

RC BplusTreeHandler::recover_update_root_page(BplusTreeMiniTransaction &mtr, PageNum root_page_num)
{
  update_root_page_num_locked(mtr, root_page_num);
  return RC::SUCCESS;
}

RC BplusTreeHandler::recover_init_header_page(BplusTreeMiniTransaction &mtr, Frame *frame, const IndexFileHeader &header)
{
  IndexFileHeader *file_header = reinterpret_cast<IndexFileHeader *>(frame->data());
  memcpy(file_header, &header, sizeof(IndexFileHeader));
  file_header_ = header;
  header_dirty_ = false;
  frame->mark_dirty();

  key_comparator_.init(file_header_.attr_type, file_header_.attr_length);
  key_printer_.init(file_header_.attr_type, file_header_.attr_length);

  return RC::SUCCESS;
}

void BplusTreeHandler::update_root_page_num_locked(BplusTreeMiniTransaction &mtr, PageNum root_page_num)
{
  Frame *frame = nullptr;
  mtr.latch_memo().get_page(FIRST_INDEX_PAGE, frame);
  mtr.latch_memo().xlatch(frame);
  IndexFileHeader *file_header = reinterpret_cast<IndexFileHeader *>(frame->data());
  mtr.logger().update_root_page(frame, root_page_num, file_header->root_page);
  file_header->root_page = root_page_num;
  file_header_.root_page = root_page_num;
  header_dirty_          = true;
  frame->mark_dirty();
  LOG_DEBUG("set root page to %d", root_page_num);
}

RC BplusTreeHandler::create_new_tree(BplusTreeMiniTransaction &mtr, const char *key, const RID *rid)
{
  RC rc = RC::SUCCESS;
  if (file_header_.root_page != BP_INVALID_PAGE_NUM) {
    rc = RC::INTERNAL;
    LOG_WARN("cannot create new tree while root page is valid. root page id=%d", file_header_.root_page);
    return rc;
  }

  Frame *frame = nullptr;

  rc = mtr.latch_memo().allocate_page(frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to allocate root page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  LeafIndexNodeHandler leaf_node(mtr, file_header_, frame);
  leaf_node.init_empty();
  leaf_node.insert(0, key, (const char *)rid);
  update_root_page_num_locked(mtr, frame->page_num());
  frame->mark_dirty();

  // disk_buffer_pool_->check_all_pages_unpinned(file_id_);
  return rc;
}

MemPoolItem::item_unique_ptr BplusTreeHandler::make_key(const char *user_key, const RID &rid)
{
  MemPoolItem::item_unique_ptr key = mem_pool_item_->alloc_unique_ptr();
  if (key == nullptr) {
    LOG_WARN("Failed to alloc memory for key.");
    return nullptr;
  }
  memcpy(static_cast<char *>(key.get()), user_key, file_header_.attr_length);
  memcpy(static_cast<char *>(key.get()) + file_header_.attr_length, &rid, sizeof(rid));
  return key;
}

RC BplusTreeHandler::insert_entry(const char *user_key, const RID *rid)
{
  if (user_key == nullptr || rid == nullptr) {
    LOG_WARN("Invalid arguments, key is empty or rid is empty");
    return RC::INVALID_ARGUMENT;
  }

  MemPoolItem::item_unique_ptr pkey = make_key(user_key, *rid);
  if (pkey == nullptr) {
    LOG_WARN("Failed to alloc memory for key.");
    return RC::NOMEM;
  }

  RC rc = RC::SUCCESS;

  BplusTreeMiniTransaction mtr(*this, &rc);

  char *key = static_cast<char *>(pkey.get());

  if (is_empty()) {
    root_lock_.lock();
    if (is_empty()) {
      rc = create_new_tree(mtr, key, rid);
      root_lock_.unlock();
      return rc;
    }
    root_lock_.unlock();
  }

  Frame *frame = nullptr;

  rc = find_leaf(mtr, BplusTreeOperationType::INSERT, key, frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("Failed to find leaf %s. rc=%d:%s", rid->to_string().c_str(), rc, strrc(rc));
    return rc;
  }

  rc = insert_entry_into_leaf_node(mtr, frame, key, rid);
  if (OB_FAIL(rc)) {
    LOG_TRACE("Failed to insert into leaf of index, rid:%s. rc=%s", rid->to_string().c_str(), strrc(rc));
    return rc;
  }

  LOG_TRACE("insert entry success");
  return RC::SUCCESS;
}

RC BplusTreeHandler::get_entry(const char *user_key, int key_len, list<RID> &rids)
{
  BplusTreeScanner scanner(*this);
  RC rc = scanner.open(user_key, key_len, true /*left_inclusive*/, user_key, key_len, true /*right_inclusive*/);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open scanner. rc=%s", strrc(rc));
    return rc;
  }

  RID rid;
  while ((rc = scanner.next_entry(rid)) == RC::SUCCESS) {
    rids.push_back(rid);
  }

  scanner.close();
  if (rc != RC::RECORD_EOF) {
    LOG_WARN("scanner return error. rc=%s", strrc(rc));
  } else {
    rc = RC::SUCCESS;
  }
  return rc;
}

RC BplusTreeHandler::adjust_root(BplusTreeMiniTransaction &mtr, Frame *root_frame)
{
  LatchMemo &latch_memo = mtr.latch_memo();

  IndexNodeHandler root_node(mtr, file_header_, root_frame);
  if (root_node.is_leaf() && root_node.size() > 0) {
    root_frame->mark_dirty();
    return RC::SUCCESS;
  }

  PageNum new_root_page_num = BP_INVALID_PAGE_NUM;
  if (root_node.is_leaf()) {
    ASSERT(root_node.size() == 0, "");
    // file_header_.root_page = BP_INVALID_PAGE_NUM;
    new_root_page_num = BP_INVALID_PAGE_NUM;
  } else {
    // 根节点只有一个子节点了，需要把自己删掉，把子节点提升为根节点
    InternalIndexNodeHandler internal_node(mtr, file_header_, root_frame);

    const PageNum child_page_num = internal_node.value_at(0);
    Frame        *child_frame    = nullptr;
    RC            rc             = latch_memo.get_page(child_page_num, child_frame);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to fetch child page. page num=%d, rc=%d:%s", child_page_num, rc, strrc(rc));
      return rc;
    }

    IndexNodeHandler child_node(mtr, file_header_, child_frame);
    child_node.set_parent_page_num(BP_INVALID_PAGE_NUM);

    // file_header_.root_page = child_page_num;
    new_root_page_num = child_page_num;
  }

  update_root_page_num_locked(mtr, new_root_page_num);

  PageNum old_root_page_num = root_frame->page_num();
  latch_memo.dispose_page(old_root_page_num);
  return RC::SUCCESS;
}

template <typename IndexNodeHandlerType>
RC BplusTreeHandler::coalesce_or_redistribute(BplusTreeMiniTransaction &mtr, Frame *frame)
{
  LatchMemo &latch_memo = mtr.latch_memo();

  IndexNodeHandlerType index_node(mtr, file_header_, frame);
  if (index_node.size() >= index_node.min_size()) {
    return RC::SUCCESS;
  }

  const PageNum parent_page_num = index_node.parent_page_num();
  if (BP_INVALID_PAGE_NUM == parent_page_num) {
    // this is the root page
    if (index_node.size() > 1) {
    } else {
      // adjust the root node
      adjust_root(mtr, frame);
    }
    return RC::SUCCESS;
  }

  Frame *parent_frame = nullptr;

  RC rc = latch_memo.get_page(parent_page_num, parent_frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to fetch parent page. page id=%d, rc=%d:%s", parent_page_num, rc, strrc(rc));
    return rc;
  }

  InternalIndexNodeHandler parent_index_node(mtr, file_header_, parent_frame);

  int index = parent_index_node.lookup(key_comparator_, index_node.key_at(index_node.size() - 1));
  ASSERT(parent_index_node.value_at(index) == frame->page_num(),
         "lookup return an invalid value. index=%d, this page num=%d, but got %d",
         index, frame->page_num(), parent_index_node.value_at(index));

  PageNum neighbor_page_num;
  if (index == 0) {
    neighbor_page_num = parent_index_node.value_at(1);
  } else {
    neighbor_page_num = parent_index_node.value_at(index - 1);
  }

  Frame *neighbor_frame = nullptr;

  // 当前已经拥有了父节点的写锁，所以直接尝试获取此页面然后加锁
  rc = latch_memo.get_page(neighbor_page_num, neighbor_frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to fetch neighbor page. page id=%d, rc=%d:%s", neighbor_page_num, rc, strrc(rc));
    // do something to release resource
    return rc;
  }

  latch_memo.xlatch(neighbor_frame);

  IndexNodeHandlerType neighbor_node(mtr, file_header_, neighbor_frame);
  if (index_node.size() + neighbor_node.size() > index_node.max_size()) {
    rc = redistribute<IndexNodeHandlerType>(mtr, neighbor_frame, frame, parent_frame, index);
  } else {
    rc = coalesce<IndexNodeHandlerType>(mtr, neighbor_frame, frame, parent_frame, index);
  }

  return rc;
}

/**
 * @brief 合并两个节点
 * @details 当某个节点数据量太少时，并且它跟它的邻居加在一起都不超过最大值，就需要跟它旁边的节点做合并。
 * 可能是内部节点，也可能是叶子节点。叶子节点还需要维护next_page指针。
 * @tparam IndexNodeHandlerType 模板类，可能是内部节点，也可能是叶子节点
 * @param mtr mini transaction
 * @param neighbor_frame 要合并的邻居页面
 * @param frame 即将合并的页面
 * @param parent_frame 父节点页面
 * @param index 在父节点的哪个位置
 */
template <typename IndexNodeHandlerType>
RC BplusTreeHandler::coalesce(
    BplusTreeMiniTransaction &mtr, Frame *neighbor_frame, Frame *frame, Frame *parent_frame, int index)
{
  InternalIndexNodeHandler parent_node(mtr, file_header_, parent_frame);

  // 先区分出来左右节点
  Frame *left_frame  = nullptr;
  Frame *right_frame = nullptr;
  if (index == 0) {
    // neighbor node is at right
    left_frame  = frame;
    right_frame = neighbor_frame;
    index++;
  } else {
    left_frame  = neighbor_frame;
    right_frame = frame;
    // neighbor is at left
  }

  // 把右边节点的数据复制到左边节点上去
  IndexNodeHandlerType left_node(mtr, file_header_, left_frame);
  IndexNodeHandlerType right_node(mtr, file_header_, right_frame);

  parent_node.remove(index);
  // parent_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
  RC rc = right_node.move_to(left_node);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to move right node to left. rc=%d:%s", rc, strrc(rc));
    return rc;
  }
  // left_node.validate(key_comparator_);

  // 叶子节点维护next_page指针
  if (left_node.is_leaf()) {
    LeafIndexNodeHandler left_leaf_node(mtr, file_header_, left_frame);
    LeafIndexNodeHandler right_leaf_node(mtr, file_header_, right_frame);
    left_leaf_node.set_next_page(right_leaf_node.next_page());
  }

  // 释放右边节点
  mtr.latch_memo().dispose_page(right_frame->page_num());

  // 递归的检查父节点是否需要做合并或者重新分配节点数据
  return coalesce_or_redistribute<InternalIndexNodeHandler>(mtr, parent_frame);
}

template <typename IndexNodeHandlerType>
RC BplusTreeHandler::redistribute(BplusTreeMiniTransaction &mtr, Frame *neighbor_frame, Frame *frame, Frame *parent_frame, int index)
{
  InternalIndexNodeHandler parent_node(mtr, file_header_, parent_frame);
  IndexNodeHandlerType     neighbor_node(mtr, file_header_, neighbor_frame);
  IndexNodeHandlerType     node(mtr, file_header_, frame);
  if (neighbor_node.size() < node.size()) {
    LOG_ERROR("got invalid nodes. neighbor node size %d, this node size %d", neighbor_node.size(), node.size());
  }
  if (index == 0) {
    // the neighbor is at right
    neighbor_node.move_first_to_end(node);
    // neighbor_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
    // node.validate(key_comparator_, disk_buffer_pool_, file_id_);
    parent_node.set_key_at(index + 1, neighbor_node.key_at(0));
    // parent_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
  } else {
    // the neighbor is at left
    neighbor_node.move_last_to_front(node);
    // neighbor_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
    // node.validate(key_comparator_, disk_buffer_pool_, file_id_);
    parent_node.set_key_at(index, node.key_at(0));
    // parent_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
  }

  neighbor_frame->mark_dirty();
  frame->mark_dirty();
  parent_frame->mark_dirty();

  return RC::SUCCESS;
}

RC BplusTreeHandler::delete_entry_internal(BplusTreeMiniTransaction &mtr, Frame *leaf_frame, const char *key)
{
  LeafIndexNodeHandler leaf_index_node(mtr, file_header_, leaf_frame);

  const int remove_count = leaf_index_node.remove(key, key_comparator_);
  if (remove_count == 0) {
    LOG_TRACE("no data need to remove");
    // disk_buffer_pool_->unpin_page(leaf_frame);
    return RC::RECORD_NOT_EXIST;
  }
  // leaf_index_node.validate(key_comparator_, disk_buffer_pool_, file_id_);

  leaf_frame->mark_dirty();

  if (leaf_index_node.size() >= leaf_index_node.min_size()) {
    return RC::SUCCESS;
  }

  return coalesce_or_redistribute<LeafIndexNodeHandler>(mtr, leaf_frame);
}

RC BplusTreeHandler::delete_entry(const char *user_key, const RID *rid)
{
  MemPoolItem::item_unique_ptr pkey = mem_pool_item_->alloc_unique_ptr();
  if (nullptr == pkey) {
    LOG_WARN("Failed to alloc memory for key. size=%d", file_header_.key_length);
    return RC::NOMEM;
  }
  char *key = static_cast<char *>(pkey.get());

  memcpy(key, user_key, file_header_.attr_length);
  memcpy(key + file_header_.attr_length, rid, sizeof(*rid));

  BplusTreeOperationType op = BplusTreeOperationType::DELETE;

  RC rc = RC::SUCCESS;

  BplusTreeMiniTransaction mtr(*this, &rc);

  Frame *leaf_frame = nullptr;

  rc = find_leaf(mtr, op, key, leaf_frame);
  if (rc == RC::EMPTY) {
    rc = RC::RECORD_NOT_EXIST;
    return rc;
  }

  if (OB_FAIL(rc)) {
    LOG_WARN("failed to find leaf page. rc =%s", strrc(rc));
    return rc;
  }

  rc = delete_entry_internal(mtr, leaf_frame, key);
  return rc;
}

////////////////////////////////////////////////////////////////////////////////

BplusTreeScanner::BplusTreeScanner(BplusTreeHandler &tree_handler)
    : tree_handler_(tree_handler), mtr_(tree_handler)
{}

BplusTreeScanner::~BplusTreeScanner() { close(); }

RC BplusTreeScanner::open(const char *left_user_key, int left_len, bool left_inclusive, const char *right_user_key,
    int right_len, bool right_inclusive)
{
  RC rc = RC::SUCCESS;
  if (inited_) {
    LOG_WARN("tree scanner has been inited");
    return RC::INTERNAL;
  }

  inited_        = true;
  first_emitted_ = false;

  LatchMemo &latch_memo = mtr_.latch_memo();

  // 校验输入的键值是否是合法范围
  if (left_user_key && right_user_key) {
    const auto &attr_comparator = tree_handler_.key_comparator_.attr_comparator();
    const int   result          = attr_comparator(left_user_key, right_user_key);
    if (result > 0 ||  // left < right
                       // left == right but is (left,right)/[left,right) or (left,right]
        (result == 0 && (left_inclusive == false || right_inclusive == false))) {
      return RC::INVALID_ARGUMENT;
    }
  }

  if (nullptr == left_user_key) {
    rc = tree_handler_.left_most_page(mtr_, current_frame_);
    if (OB_FAIL(rc)) {
      if (rc == RC::EMPTY) {
        current_frame_ = nullptr;
        return RC::SUCCESS;
      }
      
      LOG_WARN("failed to find left most page. rc=%s", strrc(rc));
      return rc;
    }

    iter_index_ = 0;
  } else {

    char *fixed_left_key = const_cast<char *>(left_user_key);
    if (tree_handler_.file_header_.attr_type == AttrType::CHARS) {
      bool should_inclusive_after_fix = false;
      rc = fix_user_key(left_user_key, left_len, true /*greater*/, &fixed_left_key, &should_inclusive_after_fix);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to fix left user key. rc=%s", strrc(rc));
        return rc;
      }

      if (should_inclusive_after_fix) {
        left_inclusive = true;
      }
    }

    MemPoolItem::item_unique_ptr left_pkey;
    if (left_inclusive) {
      left_pkey = tree_handler_.make_key(fixed_left_key, *RID::min());
    } else {
      left_pkey = tree_handler_.make_key(fixed_left_key, *RID::max());
    }

    const char *left_key = (const char *)left_pkey.get();

    if (fixed_left_key != left_user_key) {
      delete[] fixed_left_key;
      fixed_left_key = nullptr;
    }

    rc = tree_handler_.find_leaf(mtr_, BplusTreeOperationType::READ, left_key, current_frame_);
    if (rc == RC::EMPTY) {
      rc             = RC::SUCCESS;
      current_frame_ = nullptr;
      return rc;
    } else if (OB_FAIL(rc)) {
      LOG_WARN("failed to find left page. rc=%s", strrc(rc));
      return rc;
    }

    LeafIndexNodeHandler left_node(mtr_, tree_handler_.file_header_, current_frame_);
    int                  left_index = left_node.lookup(tree_handler_.key_comparator_, left_key);
    // lookup 返回的是适合插入的位置，还需要判断一下是否在合适的边界范围内
    if (left_index >= left_node.size()) {  // 超出了当前页，就需要向后移动一个位置
      const PageNum next_page_num = left_node.next_page();
      if (next_page_num == BP_INVALID_PAGE_NUM) {  // 这里已经是最后一页，说明当前扫描，没有数据
        latch_memo.release();
        current_frame_ = nullptr;
        return RC::SUCCESS;
      }

      rc = latch_memo.get_page(next_page_num, current_frame_);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to fetch next page. page num=%d, rc=%s", next_page_num, strrc(rc));
        return rc;
      }
      latch_memo.slatch(current_frame_);

      left_index = 0;
    }
    iter_index_ = left_index;
  }

  // 没有指定右边界范围，那么就返回右边界最大值
  if (nullptr == right_user_key) {
    right_key_ = nullptr;
  } else {

    char *fixed_right_key          = const_cast<char *>(right_user_key);
    bool  should_include_after_fix = false;
    if (tree_handler_.file_header_.attr_type == AttrType::CHARS) {
      rc = fix_user_key(right_user_key, right_len, false /*want_greater*/, &fixed_right_key, &should_include_after_fix);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to fix right user key. rc=%s", strrc(rc));
        return rc;
      }

      if (should_include_after_fix) {
        right_inclusive = true;
      }
    }
    if (right_inclusive) {
      right_key_ = tree_handler_.make_key(fixed_right_key, *RID::max());
    } else {
      right_key_ = tree_handler_.make_key(fixed_right_key, *RID::min());
    }

    if (fixed_right_key != right_user_key) {
      delete[] fixed_right_key;
      fixed_right_key = nullptr;
    }
  }

  if (touch_end()) {
    current_frame_ = nullptr;
  }

  return RC::SUCCESS;
}

void BplusTreeScanner::fetch_item(RID &rid)
{
  LeafIndexNodeHandler node(mtr_, tree_handler_.file_header_, current_frame_);
  memcpy(&rid, node.value_at(iter_index_), sizeof(rid));
}

bool BplusTreeScanner::touch_end()
{
  if (right_key_ == nullptr) {
    return false;
  }

  LeafIndexNodeHandler node(mtr_, tree_handler_.file_header_, current_frame_);

  const char *this_key       = node.key_at(iter_index_);
  int         compare_result = tree_handler_.key_comparator_(this_key, static_cast<char *>(right_key_.get()));
  return compare_result > 0;
}

RC BplusTreeScanner::next_entry(RID &rid)
{
  if (nullptr == current_frame_) {
    return RC::RECORD_EOF;
  }

  if (!first_emitted_) {
    fetch_item(rid);
    first_emitted_ = true;
    return RC::SUCCESS;
  }

  iter_index_++;

  LeafIndexNodeHandler node(mtr_, tree_handler_.file_header_, current_frame_);
  if (iter_index_ < node.size()) {
    if (touch_end()) {
      return RC::RECORD_EOF;
    }

    fetch_item(rid);
    return RC::SUCCESS;
  }

  RC      rc            = RC::SUCCESS;
  PageNum next_page_num = node.next_page();
  if (BP_INVALID_PAGE_NUM == next_page_num) {
    return RC::RECORD_EOF;
  }

  LatchMemo &latch_memo = mtr_.latch_memo();

  const int memo_point = latch_memo.memo_point();
  rc                   = latch_memo.get_page(next_page_num, current_frame_);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to get next page. page num=%d, rc=%s", next_page_num, strrc(rc));
    return rc;
  }

  /**
   * 如果这里直接去加锁，那可能会造成死锁
   * 因为这里访问页面的方式顺序与插入、删除的顺序不一样
   * 如果加锁失败，就由上层做重试
   */
  bool locked = latch_memo.try_slatch(current_frame_);
  if (!locked) {
    return RC::LOCKED_NEED_WAIT;
  }

  latch_memo.release_to(memo_point);
  iter_index_ = -1;  // `next` will add 1
  return next_entry(rid);
}

RC BplusTreeScanner::close()
{
  inited_ = false;
  LOG_TRACE("bplus tree scanner closed");
  return RC::SUCCESS;
}

RC BplusTreeScanner::fix_user_key(
    const char *user_key, int key_len, bool want_greater, char **fixed_key, bool *should_inclusive)
{
  if (nullptr == fixed_key || nullptr == should_inclusive) {
    return RC::INVALID_ARGUMENT;
  }

  // 这里很粗暴，变长字段才需要做调整，其它默认都不需要做调整
  assert(tree_handler_.file_header_.attr_type == AttrType::CHARS);
  assert(strlen(user_key) >= static_cast<size_t>(key_len));

  *should_inclusive = false;

  int32_t attr_length = tree_handler_.file_header_.attr_length;
  char   *key_buf     = new char[attr_length];
  if (nullptr == key_buf) {
    return RC::NOMEM;
  }

  if (key_len <= attr_length) {
    memcpy(key_buf, user_key, key_len);
    memset(key_buf + key_len, 0, attr_length - key_len);

    *fixed_key = key_buf;
    return RC::SUCCESS;
  }

  // key_len > attr_length
  memcpy(key_buf, user_key, attr_length);

  char c = user_key[attr_length];
  if (c == 0) {
    *fixed_key = key_buf;
    return RC::SUCCESS;
  }

  // 扫描 >=/> user_key 的数据
  // 示例：>=/> ABCD1 的数据，attr_length=4,
  //      等价于扫描 >= ABCE 的数据
  // 如果是扫描 <=/< user_key的数据
  // 示例：<=/< ABCD1  <==> <= ABCD  (attr_length=4)
  // NOTE: 假设都是普通的ASCII字符，不包含二进制字符，使用char不会溢出
  *should_inclusive = true;
  if (want_greater) {
    key_buf[attr_length - 1]++;
  }

  *fixed_key = key_buf;
  return RC::SUCCESS;
}
