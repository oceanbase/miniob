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
#include "storage/index/bplus_tree.h"
#include "common/lang/lower_bound.h"
#include "common/log/log.h"
#include "sql/parser/parse_defs.h"
#include "storage/buffer/disk_buffer_pool.h"

using namespace std;
using namespace common;

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
IndexNodeHandler::IndexNodeHandler(const IndexFileHeader &header, Frame *frame)
    : header_(header), page_num_(frame->page_num()), node_((IndexNode *)frame->data())
{}

bool IndexNodeHandler::is_leaf() const { return node_->is_leaf; }
void IndexNodeHandler::init_empty(bool leaf)
{
  node_->is_leaf = leaf;
  node_->key_num = 0;
  node_->parent  = BP_INVALID_PAGE_NUM;
}
PageNum IndexNodeHandler::page_num() const { return page_num_; }

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

void IndexNodeHandler::increase_size(int n) { node_->key_num += n; }

PageNum IndexNodeHandler::parent_page_num() const { return node_->parent; }

void IndexNodeHandler::set_parent_page_num(PageNum page_num) { this->node_->parent = page_num; }

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

std::string to_string(const IndexNodeHandler &handler)
{
  std::stringstream ss;

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

/////////////////////////////////////////////////////////////////////////////////
LeafIndexNodeHandler::LeafIndexNodeHandler(const IndexFileHeader &header, Frame *frame)
    : IndexNodeHandler(header, frame), leaf_node_((LeafIndexNode *)frame->data())
{}

void LeafIndexNodeHandler::init_empty()
{
  IndexNodeHandler::init_empty(true);
  leaf_node_->next_brother = BP_INVALID_PAGE_NUM;
}

void LeafIndexNodeHandler::set_next_page(PageNum page_num) { leaf_node_->next_brother = page_num; }

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

void LeafIndexNodeHandler::insert(int index, const char *key, const char *value)
{
  if (index < size()) {
    memmove(__item_at(index + 1), __item_at(index), (static_cast<size_t>(size()) - index) * item_size());
  }
  memcpy(__item_at(index), key, key_size());
  memcpy(__item_at(index) + key_size(), value, value_size());
  increase_size(1);
}
void LeafIndexNodeHandler::remove(int index)
{
  assert(index >= 0 && index < size());
  if (index < size() - 1) {
    memmove(__item_at(index), __item_at(index + 1), (static_cast<size_t>(size()) - index - 1) * item_size());
  }
  increase_size(-1);
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

RC LeafIndexNodeHandler::move_half_to(LeafIndexNodeHandler &other, DiskBufferPool *bp)
{
  const int size       = this->size();
  const int move_index = size / 2;

  memcpy(other.__item_at(0), this->__item_at(move_index), static_cast<size_t>(item_size()) * (size - move_index));
  other.increase_size(size - move_index);
  this->increase_size(-(size - move_index));
  return RC::SUCCESS;
}
RC LeafIndexNodeHandler::move_first_to_end(LeafIndexNodeHandler &other, DiskBufferPool *disk_buffer_pool)
{
  other.append(__item_at(0));

  if (size() >= 1) {
    memmove(__item_at(0), __item_at(1), (static_cast<size_t>(size()) - 1) * item_size());
  }
  increase_size(-1);
  return RC::SUCCESS;
}

RC LeafIndexNodeHandler::move_last_to_front(LeafIndexNodeHandler &other, DiskBufferPool *bp)
{
  other.preappend(__item_at(size() - 1));

  increase_size(-1);
  return RC::SUCCESS;
}
/**
 * move all items to left page
 */
RC LeafIndexNodeHandler::move_to(LeafIndexNodeHandler &other, DiskBufferPool *bp)
{
  memcpy(other.__item_at(other.size()), this->__item_at(0), static_cast<size_t>(this->size()) * item_size());
  other.increase_size(this->size());
  this->increase_size(-this->size());

  other.set_next_page(this->next_page());
  return RC::SUCCESS;
}

void LeafIndexNodeHandler::append(const char *item)
{
  memcpy(__item_at(size()), item, item_size());
  increase_size(1);
}

void LeafIndexNodeHandler::preappend(const char *item)
{
  if (size() > 0) {
    memmove(__item_at(1), __item_at(0), static_cast<size_t>(size()) * item_size());
  }
  memcpy(__item_at(0), item, item_size());
  increase_size(1);
}

char *LeafIndexNodeHandler::__item_at(int index) const { return leaf_node_->array + (index * item_size()); }
char *LeafIndexNodeHandler::__key_at(int index) const { return __item_at(index); }
char *LeafIndexNodeHandler::__value_at(int index) const { return __item_at(index) + key_size(); }

std::string to_string(const LeafIndexNodeHandler &handler, const KeyPrinter &printer)
{
  std::stringstream ss;
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

  Frame *parent_frame;
  RC     rc = bp->get_this_page(parent_page_num, &parent_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch parent page. page num=%d, rc=%d:%s", parent_page_num, rc, strrc(rc));
    return false;
  }

  InternalIndexNodeHandler parent_node(header_, parent_frame);
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
InternalIndexNodeHandler::InternalIndexNodeHandler(const IndexFileHeader &header, Frame *frame)
    : IndexNodeHandler(header, frame), internal_node_((InternalIndexNode *)frame->data())
{}

std::string to_string(const InternalIndexNodeHandler &node, const KeyPrinter &printer)
{
  std::stringstream ss;
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

void InternalIndexNodeHandler::init_empty() { IndexNodeHandler::init_empty(false); }
void InternalIndexNodeHandler::create_new_root(PageNum first_page_num, const char *key, PageNum page_num)
{
  memset(__key_at(0), 0, key_size());
  memcpy(__value_at(0), &first_page_num, value_size());
  memcpy(__item_at(1), key, key_size());
  memcpy(__value_at(1), &page_num, value_size());
  increase_size(2);
}

/**
 * insert one entry
 * the entry to be inserted will never at the first slot.
 * the right child page after split will always have bigger keys.
 */
void InternalIndexNodeHandler::insert(const char *key, PageNum page_num, const KeyComparator &comparator)
{
  int insert_position = -1;
  lookup(comparator, key, nullptr, &insert_position);
  if (insert_position < size()) {
    memmove(__item_at(insert_position + 1),
        __item_at(insert_position),
        (static_cast<size_t>(size()) - insert_position) * item_size());
  }
  memcpy(__item_at(insert_position), key, key_size());
  memcpy(__value_at(insert_position), &page_num, value_size());
  increase_size(1);
}

RC InternalIndexNodeHandler::move_half_to(InternalIndexNodeHandler &other, DiskBufferPool *bp)
{
  const int size       = this->size();
  const int move_index = size / 2;
  RC        rc         = other.copy_from(this->__item_at(move_index), size - move_index, bp);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to copy item to new node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

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
  if (index < size() - 1) {
    memmove(__item_at(index), __item_at(index + 1), (static_cast<size_t>(size()) - index - 1) * item_size());
  }
  increase_size(-1);
}

RC InternalIndexNodeHandler::move_to(InternalIndexNodeHandler &other, DiskBufferPool *disk_buffer_pool)
{
  RC rc = other.copy_from(__item_at(0), size(), disk_buffer_pool);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to copy items to other node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  increase_size(-this->size());
  return RC::SUCCESS;
}

RC InternalIndexNodeHandler::move_first_to_end(InternalIndexNodeHandler &other, DiskBufferPool *disk_buffer_pool)
{
  RC rc = other.append(__item_at(0), disk_buffer_pool);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to append item to others.");
    return rc;
  }

  if (size() >= 1) {
    memmove(__item_at(0), __item_at(1), (static_cast<size_t>(size()) - 1) * item_size());
  }
  increase_size(-1);
  return rc;
}

RC InternalIndexNodeHandler::move_last_to_front(InternalIndexNodeHandler &other, DiskBufferPool *bp)
{
  RC rc = other.preappend(__item_at(size() - 1), bp);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to preappend to others");
    return rc;
  }

  increase_size(-1);
  return rc;
}
/**
 * copy items from other node to self's right
 */
RC InternalIndexNodeHandler::copy_from(const char *items, int num, DiskBufferPool *disk_buffer_pool)
{
  memcpy(__item_at(this->size()), items, static_cast<size_t>(num) * item_size());

  RC      rc            = RC::SUCCESS;
  PageNum this_page_num = this->page_num();
  Frame  *frame         = nullptr;
  for (int i = 0; i < num; i++) {
    const PageNum page_num = *(const PageNum *)((items + i * item_size()) + key_size());
    rc                     = disk_buffer_pool->get_this_page(page_num, &frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to set child's page num. child page num:%d, this page num=%d, rc=%d:%s",
               page_num, this_page_num, rc, strrc(rc));
      return rc;
    }
    IndexNodeHandler child_node(header_, frame);
    child_node.set_parent_page_num(this_page_num);
    frame->mark_dirty();
    disk_buffer_pool->unpin_page(frame);
  }
  increase_size(num);
  return rc;
}

RC InternalIndexNodeHandler::append(const char *item, DiskBufferPool *bp) { return this->copy_from(item, 1, bp); }

RC InternalIndexNodeHandler::preappend(const char *item, DiskBufferPool *bp)
{
  PageNum child_page_num = *(PageNum *)(item + key_size());
  Frame  *frame          = nullptr;

  RC rc = bp->get_this_page(child_page_num, &frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch child page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  IndexNodeHandler child_node(header_, frame);
  child_node.set_parent_page_num(this->page_num());

  frame->mark_dirty();
  bp->unpin_page(frame);

  if (this->size() > 0) {
    memmove(__item_at(1), __item_at(0), static_cast<size_t>(this->size()) * item_size());
  }

  memcpy(__item_at(0), item, item_size());
  increase_size(1);
  return RC::SUCCESS;
}

char *InternalIndexNodeHandler::__item_at(int index) const { return internal_node_->array + (index * item_size()); }

char *InternalIndexNodeHandler::__key_at(int index) const { return __item_at(index); }

char *InternalIndexNodeHandler::__value_at(int index) const { return __item_at(index) + key_size(); }

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
      Frame *child_frame;
      RC     rc = bp->get_this_page(page_num, &child_frame);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to fetch child page while validate internal page. page num=%d, rc=%d:%s", 
                 page_num, rc, strrc(rc));
      } else {
        IndexNodeHandler child_node(header_, child_frame);
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

  Frame *parent_frame;
  RC     rc = bp->get_this_page(parent_page_num, &parent_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch parent page. page num=%d, rc=%d:%s", parent_page_num, rc, strrc(rc));
    return false;
  }

  InternalIndexNodeHandler parent_node(header_, parent_frame);

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

RC BplusTreeHandler::create(const char *file_name, AttrType attr_type, int attr_length, int internal_max_size /* = -1*/,
    int leaf_max_size /* = -1 */)
{
  BufferPoolManager &bpm = BufferPoolManager::instance();

  RC rc = bpm.create_file(file_name);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to create file. file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    return rc;
  }
  LOG_INFO("Successfully create index file:%s", file_name);

  DiskBufferPool *bp = nullptr;

  rc = bpm.open_file(file_name, bp);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to open file. file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    return rc;
  }
  LOG_INFO("Successfully open index file %s.", file_name);

  Frame *header_frame;
  rc = bp->allocate_page(&header_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to allocate header page for bplus tree. rc=%d:%s", rc, strrc(rc));
    bpm.close_file(file_name);
    return rc;
  }

  if (header_frame->page_num() != FIRST_INDEX_PAGE) {
    LOG_WARN("header page num should be %d but got %d. is it a new file : %s",
             FIRST_INDEX_PAGE, header_frame->page_num(), file_name);
    bpm.close_file(file_name);
    return RC::INTERNAL;
  }

  if (internal_max_size < 0) {
    internal_max_size = calc_internal_page_capacity(attr_length);
  }
  if (leaf_max_size < 0) {
    leaf_max_size = calc_leaf_page_capacity(attr_length);
  }

  char            *pdata         = header_frame->data();
  IndexFileHeader *file_header   = (IndexFileHeader *)pdata;
  file_header->attr_length       = attr_length;
  file_header->key_length        = attr_length + sizeof(RID);
  file_header->attr_type         = attr_type;
  file_header->internal_max_size = internal_max_size;
  file_header->leaf_max_size     = leaf_max_size;
  file_header->root_page         = BP_INVALID_PAGE_NUM;

  header_frame->mark_dirty();

  disk_buffer_pool_ = bp;

  memcpy(&file_header_, pdata, sizeof(file_header_));
  header_dirty_ = false;
  bp->unpin_page(header_frame);

  mem_pool_item_ = make_unique<common::MemPoolItem>(file_name);
  if (mem_pool_item_->init(file_header->key_length) < 0) {
    LOG_WARN("Failed to init memory pool for index %s", file_name);
    close();
    return RC::NOMEM;
  }

  key_comparator_.init(file_header->attr_type, file_header->attr_length);
  key_printer_.init(file_header->attr_type, file_header->attr_length);

  this->sync();

  LOG_INFO("Successfully create index %s", file_name);
  return RC::SUCCESS;
}

RC BplusTreeHandler::open(const char *file_name)
{
  if (disk_buffer_pool_ != nullptr) {
    LOG_WARN("%s has been opened before index.open.", file_name);
    return RC::RECORD_OPENNED;
  }

  BufferPoolManager &bpm              = BufferPoolManager::instance();
  DiskBufferPool    *disk_buffer_pool = nullptr;

  RC rc = bpm.open_file(file_name, disk_buffer_pool);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to open file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    return rc;
  }

  Frame *frame = nullptr;
  rc           = disk_buffer_pool->get_this_page(FIRST_INDEX_PAGE, &frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get first page file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    bpm.close_file(file_name);
    return rc;
  }

  char *pdata = frame->data();
  memcpy(&file_header_, pdata, sizeof(IndexFileHeader));
  header_dirty_     = false;
  disk_buffer_pool_ = disk_buffer_pool;

  mem_pool_item_ = make_unique<common::MemPoolItem>(file_name);
  if (mem_pool_item_->init(file_header_.key_length) < 0) {
    LOG_WARN("Failed to init memory pool for index %s", file_name);
    close();
    return RC::NOMEM;
  }

  // close old page_handle
  disk_buffer_pool->unpin_page(frame);

  key_comparator_.init(file_header_.attr_type, file_header_.attr_length);
  key_printer_.init(file_header_.attr_type, file_header_.attr_length);
  LOG_INFO("Successfully open index %s", file_name);
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
  LeafIndexNodeHandler leaf_node(file_header_, frame);
  LOG_INFO("leaf node: %s", to_string(leaf_node, key_printer_).c_str());
  disk_buffer_pool_->unpin_page(frame);
  return RC::SUCCESS;
}

RC BplusTreeHandler::print_internal_node_recursive(Frame *frame)
{
  RC rc = RC::SUCCESS;
  LOG_INFO("bplus tree. file header: %s", file_header_.to_string().c_str());
  InternalIndexNodeHandler internal_node(file_header_, frame);
  LOG_INFO("internal node: %s", to_string(internal_node, key_printer_).c_str());

  int node_size = internal_node.size();
  for (int i = 0; i < node_size; i++) {
    PageNum page_num = internal_node.value_at(i);
    Frame  *child_frame;
    rc = disk_buffer_pool_->get_this_page(page_num, &child_frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch child page. page id=%d, rc=%d:%s", page_num, rc, strrc(rc));
      disk_buffer_pool_->unpin_page(frame);
      return rc;
    }

    IndexNodeHandler node(file_header_, child_frame);
    if (node.is_leaf()) {
      rc = print_leaf(child_frame);
    } else {
      rc = print_internal_node_recursive(child_frame);
    }
    if (rc != RC::SUCCESS) {
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
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch page. page id=%d, rc=%d:%s", page_num, rc, strrc(rc));
    return rc;
  }

  IndexNodeHandler node(file_header_, frame);
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

  LatchMemo latch_memo(disk_buffer_pool_);
  Frame    *frame = nullptr;

  RC rc = left_most_page(latch_memo, frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get left most page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  while (frame->page_num() != BP_INVALID_PAGE_NUM) {
    LeafIndexNodeHandler leaf_node(file_header_, frame);
    LOG_INFO("leaf info: %s", to_string(leaf_node, key_printer_).c_str());

    PageNum next_page_num = leaf_node.next_page();
    latch_memo.release();

    if (next_page_num == BP_INVALID_PAGE_NUM) {
      break;
    }

    rc = latch_memo.get_page(next_page_num, frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get next page. page id=%d, rc=%d:%s", next_page_num, rc, strrc(rc));
      return rc;
    }
  }
  return rc;
}

bool BplusTreeHandler::validate_node_recursive(LatchMemo &latch_memo, Frame *frame)
{
  bool             result = true;
  IndexNodeHandler node(file_header_, frame);
  if (node.is_leaf()) {
    LeafIndexNodeHandler leaf_node(file_header_, frame);
    result = leaf_node.validate(key_comparator_, disk_buffer_pool_);
  } else {
    InternalIndexNodeHandler internal_node(file_header_, frame);
    result = internal_node.validate(key_comparator_, disk_buffer_pool_);
    for (int i = 0; result && i < internal_node.size(); i++) {
      PageNum page_num = internal_node.value_at(i);
      Frame  *child_frame;
      RC      rc = latch_memo.get_page(page_num, child_frame);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to fetch child page.page id=%d, rc=%d:%s", page_num, rc, strrc(rc));
        result = false;
        break;
      }

      result = validate_node_recursive(latch_memo, child_frame);
    }
  }

  return result;
}

bool BplusTreeHandler::validate_leaf_link(LatchMemo &latch_memo)
{
  if (is_empty()) {
    return true;
  }

  Frame *frame = nullptr;

  RC rc = left_most_page(latch_memo, frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch left most page. rc=%d:%s", rc, strrc(rc));
    return false;
  }

  LeafIndexNodeHandler leaf_node(file_header_, frame);
  PageNum              next_page_num = leaf_node.next_page();

  MemPoolItem::unique_ptr prev_key = mem_pool_item_->alloc_unique_ptr();
  memcpy(prev_key.get(), leaf_node.key_at(leaf_node.size() - 1), file_header_.key_length);

  bool result = true;
  while (result && next_page_num != BP_INVALID_PAGE_NUM) {
    rc = latch_memo.get_page(next_page_num, frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch next page. page num=%d, rc=%s", next_page_num, strrc(rc));
      return false;
    }

    LeafIndexNodeHandler leaf_node(file_header_, frame);
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

  LatchMemo latch_memo(disk_buffer_pool_);
  Frame    *frame = nullptr;
  RC        rc    = latch_memo.get_page(file_header_.root_page, frame);  // 这里仅仅调试使用，不加root锁
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch root page. page id=%d, rc=%d:%s", file_header_.root_page, rc, strrc(rc));
    return false;
  }

  if (!validate_node_recursive(latch_memo, frame) || !validate_leaf_link(latch_memo)) {
    LOG_WARN("Current B+ Tree is invalid");
    print_tree();
    return false;
  }

  LOG_INFO("great! current tree is valid");
  return true;
}

bool BplusTreeHandler::is_empty() const { return file_header_.root_page == BP_INVALID_PAGE_NUM; }

RC BplusTreeHandler::find_leaf(LatchMemo &latch_memo, BplusTreeOperationType op, const char *key, Frame *&frame)
{
  auto child_page_getter = [this, key](InternalIndexNodeHandler &internal_node) {
    return internal_node.value_at(internal_node.lookup(key_comparator_, key));
  };
  return find_leaf_internal(latch_memo, op, child_page_getter, frame);
}

RC BplusTreeHandler::left_most_page(LatchMemo &latch_memo, Frame *&frame)
{
  auto child_page_getter = [](InternalIndexNodeHandler &internal_node) { return internal_node.value_at(0); };
  return find_leaf_internal(latch_memo, BplusTreeOperationType::READ, child_page_getter, frame);
}

RC BplusTreeHandler::find_leaf_internal(LatchMemo &latch_memo, BplusTreeOperationType op,
    const std::function<PageNum(InternalIndexNodeHandler &)> &child_page_getter, Frame *&frame)
{
  // root locked
  if (op != BplusTreeOperationType::READ) {
    latch_memo.xlatch(&root_lock_);
  } else {
    latch_memo.slatch(&root_lock_);
  }

  if (is_empty()) {
    return RC::EMPTY;
  }

  RC rc = crabing_protocal_fetch_page(latch_memo, op, file_header_.root_page, true /* is_root_node */, frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch root page. page id=%d, rc=%d:%s", file_header_.root_page, rc, strrc(rc));
    return rc;
  }

  IndexNode *node = (IndexNode *)frame->data();
  PageNum    next_page_id;
  for (; !node->is_leaf;) {
    InternalIndexNodeHandler internal_node(file_header_, frame);
    next_page_id = child_page_getter(internal_node);
    rc           = crabing_protocal_fetch_page(latch_memo, op, next_page_id, false /* is_root_node */, frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to load page page_num:%d. rc=%s", next_page_id, strrc(rc));
      return rc;
    }

    node = (IndexNode *)frame->data();
  }
  return RC::SUCCESS;
}

RC BplusTreeHandler::crabing_protocal_fetch_page(
    LatchMemo &latch_memo, BplusTreeOperationType op, PageNum page_num, bool is_root_node, Frame *&frame)
{
  bool      readonly   = (op == BplusTreeOperationType::READ);
  const int memo_point = latch_memo.memo_point();

  RC rc = latch_memo.get_page(page_num, frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get frame. pageNum=%d, rc=%s", page_num, strrc(rc));
    return rc;
  }

  LatchMemoType latch_type = readonly ? LatchMemoType::SHARED : LatchMemoType::EXCLUSIVE;
  latch_memo.latch(frame, latch_type);
  IndexNodeHandler index_node(file_header_, frame);
  if (index_node.is_safe(op, is_root_node)) {
    latch_memo.release_to(memo_point);  // 当前节点不会分裂或合并，可以将前面的锁都释放掉
  }
  return rc;
}

RC BplusTreeHandler::insert_entry_into_leaf_node(LatchMemo &latch_memo, Frame *frame, const char *key, const RID *rid)
{
  LeafIndexNodeHandler leaf_node(file_header_, frame);
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
  RC     rc        = split<LeafIndexNodeHandler>(latch_memo, frame, new_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to split leaf node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  LeafIndexNodeHandler new_index_node(file_header_, new_frame);
  new_index_node.set_next_page(leaf_node.next_page());
  new_index_node.set_parent_page_num(leaf_node.parent_page_num());
  leaf_node.set_next_page(new_frame->page_num());

  if (insert_position < leaf_node.size()) {
    leaf_node.insert(insert_position, key, (const char *)rid);
  } else {
    new_index_node.insert(insert_position - leaf_node.size(), key, (const char *)rid);
  }

  return insert_entry_into_parent(latch_memo, frame, new_frame, new_index_node.key_at(0));
}

RC BplusTreeHandler::insert_entry_into_parent(LatchMemo &latch_memo, Frame *frame, Frame *new_frame, const char *key)
{
  RC rc = RC::SUCCESS;

  IndexNodeHandler node_handler(file_header_, frame);
  IndexNodeHandler new_node_handler(file_header_, new_frame);
  PageNum          parent_page_num = node_handler.parent_page_num();

  if (parent_page_num == BP_INVALID_PAGE_NUM) {

    // create new root page
    Frame *root_frame;
    rc = disk_buffer_pool_->allocate_page(&root_frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to allocate new root page. rc=%d:%s", rc, strrc(rc));
      return rc;
    }

    InternalIndexNodeHandler root_node(file_header_, root_frame);
    root_node.init_empty();
    root_node.create_new_root(frame->page_num(), key, new_frame->page_num());
    node_handler.set_parent_page_num(root_frame->page_num());
    new_node_handler.set_parent_page_num(root_frame->page_num());

    frame->mark_dirty();
    new_frame->mark_dirty();
    // disk_buffer_pool_->unpin_page(frame);
    // disk_buffer_pool_->unpin_page(new_frame);

    root_frame->write_latch();  // 在root页面更新之后，别人就可以访问到了，这时候就要加上锁
    update_root_page_num_locked(root_frame->page_num());
    root_frame->mark_dirty();
    root_frame->write_unlatch();
    disk_buffer_pool_->unpin_page(root_frame);

    return RC::SUCCESS;

  } else {

    Frame *parent_frame = nullptr;

    rc = latch_memo.get_page(parent_page_num, parent_frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to insert entry into leaf. rc=%d:%s", rc, strrc(rc));
      // we should do some things to recover
      return rc;
    }

    // 在第一次遍历这个页面时，我们已经拿到parent frame的write latch，所以这里不再去加锁
    InternalIndexNodeHandler parent_node(file_header_, parent_frame);

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

      rc = split<InternalIndexNodeHandler>(latch_memo, parent_frame, new_parent_frame);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to split internal node. rc=%d:%s", rc, strrc(rc));
        // disk_buffer_pool_->unpin_page(frame);
        // disk_buffer_pool_->unpin_page(new_frame);
        // disk_buffer_pool_->unpin_page(parent_frame);
      } else {
        // insert into left or right ? decide by key compare result
        InternalIndexNodeHandler new_node(file_header_, new_parent_frame);
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
        rc = insert_entry_into_parent(latch_memo, parent_frame, new_parent_frame, new_node.key_at(0));
      }
    }
  }
  return rc;
}

/**
 * split one full node into two
 */
template <typename IndexNodeHandlerType>
RC BplusTreeHandler::split(LatchMemo &latch_memo, Frame *frame, Frame *&new_frame)
{
  IndexNodeHandlerType old_node(file_header_, frame);

  // add a new node
  RC rc = latch_memo.allocate_page(new_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to split index page due to failed to allocate page, rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  latch_memo.xlatch(new_frame);

  IndexNodeHandlerType new_node(file_header_, new_frame);
  new_node.init_empty();
  new_node.set_parent_page_num(old_node.parent_page_num());

  old_node.move_half_to(new_node, disk_buffer_pool_);  // TODO remove disk buffer pool

  frame->mark_dirty();
  new_frame->mark_dirty();
  return RC::SUCCESS;
}

void BplusTreeHandler::update_root_page_num_locked(PageNum root_page_num)
{
  file_header_.root_page = root_page_num;
  header_dirty_          = true;
  LOG_DEBUG("set root page to %d", root_page_num);
}

RC BplusTreeHandler::create_new_tree(const char *key, const RID *rid)
{
  RC rc = RC::SUCCESS;
  if (file_header_.root_page != BP_INVALID_PAGE_NUM) {
    rc = RC::INTERNAL;
    LOG_WARN("cannot create new tree while root page is valid. root page id=%d", file_header_.root_page);
    return rc;
  }

  Frame *frame = nullptr;

  rc = disk_buffer_pool_->allocate_page(&frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to allocate root page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  LeafIndexNodeHandler leaf_node(file_header_, frame);
  leaf_node.init_empty();
  leaf_node.insert(0, key, (const char *)rid);
  update_root_page_num_locked(frame->page_num());
  frame->mark_dirty();
  disk_buffer_pool_->unpin_page(frame);

  // disk_buffer_pool_->check_all_pages_unpinned(file_id_);
  return rc;
}

MemPoolItem::unique_ptr BplusTreeHandler::make_key(const char *user_key, const RID &rid)
{
  MemPoolItem::unique_ptr key = mem_pool_item_->alloc_unique_ptr();
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

  MemPoolItem::unique_ptr pkey = make_key(user_key, *rid);
  if (pkey == nullptr) {
    LOG_WARN("Failed to alloc memory for key.");
    return RC::NOMEM;
  }

  char *key = static_cast<char *>(pkey.get());

  if (is_empty()) {
    root_lock_.lock();
    if (is_empty()) {
      RC rc = create_new_tree(key, rid);
      root_lock_.unlock();
      return rc;
    }
    root_lock_.unlock();
  }

  LatchMemo latch_memo(disk_buffer_pool_);

  Frame *frame = nullptr;
  RC     rc    = find_leaf(latch_memo, BplusTreeOperationType::INSERT, key, frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to find leaf %s. rc=%d:%s", rid->to_string().c_str(), rc, strrc(rc));
    return rc;
  }

  rc = insert_entry_into_leaf_node(latch_memo, frame, key, rid);
  if (rc != RC::SUCCESS) {
    LOG_TRACE("Failed to insert into leaf of index, rid:%s. rc=%s", rid->to_string().c_str(), strrc(rc));
    return rc;
  }

  LOG_TRACE("insert entry success");
  return RC::SUCCESS;
}

RC BplusTreeHandler::get_entry(const char *user_key, int key_len, std::list<RID> &rids)
{
  BplusTreeScanner scanner(*this);
  RC rc = scanner.open(user_key, key_len, true /*left_inclusive*/, user_key, key_len, true /*right_inclusive*/);
  if (rc != RC::SUCCESS) {
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

RC BplusTreeHandler::adjust_root(LatchMemo &latch_memo, Frame *root_frame)
{
  IndexNodeHandler root_node(file_header_, root_frame);
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
    InternalIndexNodeHandler internal_node(file_header_, root_frame);

    const PageNum child_page_num = internal_node.value_at(0);
    Frame        *child_frame    = nullptr;
    RC            rc             = latch_memo.get_page(child_page_num, child_frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch child page. page num=%d, rc=%d:%s", child_page_num, rc, strrc(rc));
      return rc;
    }

    IndexNodeHandler child_node(file_header_, child_frame);
    child_node.set_parent_page_num(BP_INVALID_PAGE_NUM);

    // file_header_.root_page = child_page_num;
    new_root_page_num = child_page_num;
  }

  update_root_page_num_locked(new_root_page_num);

  PageNum old_root_page_num = root_frame->page_num();
  latch_memo.dispose_page(old_root_page_num);
  return RC::SUCCESS;
}

template <typename IndexNodeHandlerType>
RC BplusTreeHandler::coalesce_or_redistribute(LatchMemo &latch_memo, Frame *frame)
{
  IndexNodeHandlerType index_node(file_header_, frame);
  if (index_node.size() >= index_node.min_size()) {
    return RC::SUCCESS;
  }

  const PageNum parent_page_num = index_node.parent_page_num();
  if (BP_INVALID_PAGE_NUM == parent_page_num) {
    // this is the root page
    if (index_node.size() > 1) {
    } else {
      // adjust the root node
      adjust_root(latch_memo, frame);
    }
    return RC::SUCCESS;
  }

  Frame *parent_frame = nullptr;

  RC rc = latch_memo.get_page(parent_page_num, parent_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch parent page. page id=%d, rc=%d:%s", parent_page_num, rc, strrc(rc));
    return rc;
  }

  InternalIndexNodeHandler parent_index_node(file_header_, parent_frame);

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
  rc                    = latch_memo.get_page(
      neighbor_page_num, neighbor_frame);  // 当前已经拥有了父节点的写锁，所以直接尝试获取此页面然后加锁
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch neighbor page. page id=%d, rc=%d:%s", neighbor_page_num, rc, strrc(rc));
    // do something to release resource
    return rc;
  }

  latch_memo.xlatch(neighbor_frame);

  IndexNodeHandlerType neighbor_node(file_header_, neighbor_frame);
  if (index_node.size() + neighbor_node.size() > index_node.max_size()) {
    rc = redistribute<IndexNodeHandlerType>(neighbor_frame, frame, parent_frame, index);
  } else {
    rc = coalesce<IndexNodeHandlerType>(latch_memo, neighbor_frame, frame, parent_frame, index);
  }

  return rc;
}

template <typename IndexNodeHandlerType>
RC BplusTreeHandler::coalesce(
    LatchMemo &latch_memo, Frame *neighbor_frame, Frame *frame, Frame *parent_frame, int index)
{
  InternalIndexNodeHandler parent_node(file_header_, parent_frame);

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

  IndexNodeHandlerType left_node(file_header_, left_frame);
  IndexNodeHandlerType right_node(file_header_, right_frame);

  parent_node.remove(index);
  // parent_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
  RC rc = right_node.move_to(left_node, disk_buffer_pool_);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to move right node to left. rc=%d:%s", rc, strrc(rc));
    return rc;
  }
  // left_node.validate(key_comparator_);

  if (left_node.is_leaf()) {
    LeafIndexNodeHandler left_leaf_node(file_header_, left_frame);
    LeafIndexNodeHandler right_leaf_node(file_header_, right_frame);
    left_leaf_node.set_next_page(right_leaf_node.next_page());
  }

  latch_memo.dispose_page(right_frame->page_num());
  return coalesce_or_redistribute<InternalIndexNodeHandler>(latch_memo, parent_frame);
}

template <typename IndexNodeHandlerType>
RC BplusTreeHandler::redistribute(Frame *neighbor_frame, Frame *frame, Frame *parent_frame, int index)
{
  InternalIndexNodeHandler parent_node(file_header_, parent_frame);
  IndexNodeHandlerType     neighbor_node(file_header_, neighbor_frame);
  IndexNodeHandlerType     node(file_header_, frame);
  if (neighbor_node.size() < node.size()) {
    LOG_ERROR("got invalid nodes. neighbor node size %d, this node size %d", neighbor_node.size(), node.size());
  }
  if (index == 0) {
    // the neighbor is at right
    neighbor_node.move_first_to_end(node, disk_buffer_pool_);
    // neighbor_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
    // node.validate(key_comparator_, disk_buffer_pool_, file_id_);
    parent_node.set_key_at(index + 1, neighbor_node.key_at(0));
    // parent_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
  } else {
    // the neighbor is at left
    neighbor_node.move_last_to_front(node, disk_buffer_pool_);
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

RC BplusTreeHandler::delete_entry_internal(LatchMemo &latch_memo, Frame *leaf_frame, const char *key)
{
  LeafIndexNodeHandler leaf_index_node(file_header_, leaf_frame);

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

  return coalesce_or_redistribute<LeafIndexNodeHandler>(latch_memo, leaf_frame);
}

RC BplusTreeHandler::delete_entry(const char *user_key, const RID *rid)
{
  MemPoolItem::unique_ptr pkey = mem_pool_item_->alloc_unique_ptr();
  if (nullptr == pkey) {
    LOG_WARN("Failed to alloc memory for key. size=%d", file_header_.key_length);
    return RC::NOMEM;
  }
  char *key = static_cast<char *>(pkey.get());

  memcpy(key, user_key, file_header_.attr_length);
  memcpy(key + file_header_.attr_length, rid, sizeof(*rid));

  BplusTreeOperationType op = BplusTreeOperationType::DELETE;
  LatchMemo              latch_memo(disk_buffer_pool_);

  Frame *leaf_frame = nullptr;

  RC rc = find_leaf(latch_memo, op, key, leaf_frame);
  if (rc == RC::EMPTY) {
    rc = RC::RECORD_NOT_EXIST;
    return rc;
  }

  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to find leaf page. rc =%s", strrc(rc));
    return rc;
  }

  return delete_entry_internal(latch_memo, leaf_frame, key);
}

////////////////////////////////////////////////////////////////////////////////

BplusTreeScanner::BplusTreeScanner(BplusTreeHandler &tree_handler)
    : tree_handler_(tree_handler), latch_memo_(tree_handler.disk_buffer_pool_)
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
    rc = tree_handler_.left_most_page(latch_memo_, current_frame_);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to find left most page. rc=%s", strrc(rc));
      return rc;
    }

    iter_index_ = 0;
  } else {

    char *fixed_left_key = const_cast<char *>(left_user_key);
    if (tree_handler_.file_header_.attr_type == CHARS) {
      bool should_inclusive_after_fix = false;
      rc = fix_user_key(left_user_key, left_len, true /*greater*/, &fixed_left_key, &should_inclusive_after_fix);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to fix left user key. rc=%s", strrc(rc));
        return rc;
      }

      if (should_inclusive_after_fix) {
        left_inclusive = true;
      }
    }

    MemPoolItem::unique_ptr left_pkey;
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

    rc = tree_handler_.find_leaf(latch_memo_, BplusTreeOperationType::READ, left_key, current_frame_);
    if (rc == RC::EMPTY) {
      rc             = RC::SUCCESS;
      current_frame_ = nullptr;
      return rc;
    } else if (rc != RC::SUCCESS) {
      LOG_WARN("failed to find left page. rc=%s", strrc(rc));
      return rc;
    }

    LeafIndexNodeHandler left_node(tree_handler_.file_header_, current_frame_);
    int                  left_index = left_node.lookup(tree_handler_.key_comparator_, left_key);
    // lookup 返回的是适合插入的位置，还需要判断一下是否在合适的边界范围内
    if (left_index >= left_node.size()) {  // 超出了当前页，就需要向后移动一个位置
      const PageNum next_page_num = left_node.next_page();
      if (next_page_num == BP_INVALID_PAGE_NUM) {  // 这里已经是最后一页，说明当前扫描，没有数据
        latch_memo_.release();
        current_frame_ = nullptr;
        return RC::SUCCESS;
      }

      rc = latch_memo_.get_page(next_page_num, current_frame_);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to fetch next page. page num=%d, rc=%s", next_page_num, strrc(rc));
        return rc;
      }
      latch_memo_.slatch(current_frame_);

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
    if (tree_handler_.file_header_.attr_type == CHARS) {
      rc = fix_user_key(right_user_key, right_len, false /*want_greater*/, &fixed_right_key, &should_include_after_fix);
      if (rc != RC::SUCCESS) {
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
  LeafIndexNodeHandler node(tree_handler_.file_header_, current_frame_);
  memcpy(&rid, node.value_at(iter_index_), sizeof(rid));
}

bool BplusTreeScanner::touch_end()
{
  if (right_key_ == nullptr) {
    return false;
  }

  LeafIndexNodeHandler node(tree_handler_.file_header_, current_frame_);

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

  LeafIndexNodeHandler node(tree_handler_.file_header_, current_frame_);
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

  const int memo_point = latch_memo_.memo_point();
  rc                   = latch_memo_.get_page(next_page_num, current_frame_);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get next page. page num=%d, rc=%s", next_page_num, strrc(rc));
    return rc;
  }

  /**
   * 如果这里直接去加锁，那可能会造成死锁
   * 因为这里访问页面的方式顺序与插入、删除的顺序不一样
   * 如果加锁失败，就由上层做重试
   */
  bool locked = latch_memo_.try_slatch(current_frame_);
  if (!locked) {
    return RC::LOCKED_NEED_WAIT;
  }

  latch_memo_.release_to(memo_point);
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
  assert(tree_handler_.file_header_.attr_type == CHARS);
  assert(strlen(user_key) >= static_cast<size_t>(key_len));

  *should_inclusive = false;

  int32_t attr_length = tree_handler_.file_header_.attr_length;
  char   *key_buf     = new (std::nothrow) char[attr_length];
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
