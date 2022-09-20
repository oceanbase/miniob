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
#include "storage/default/disk_buffer_pool.h"
#include "rc.h"
#include "common/log/log.h"
#include "sql/parser/parse_defs.h"
#include "common/lang/lower_bound.h"

#define FIRST_INDEX_PAGE 1

int calc_internal_page_capacity(int attr_length)
{
  int item_size = attr_length + sizeof(RID) + sizeof(PageNum);

  int capacity =
    ((int)BP_PAGE_DATA_SIZE - InternalIndexNode::HEADER_SIZE) / item_size;
  return capacity;
}

int calc_leaf_page_capacity(int attr_length)
{
  int item_size = attr_length + sizeof(RID) + sizeof(RID);
  int capacity =
    ((int)BP_PAGE_DATA_SIZE - LeafIndexNode::HEADER_SIZE) / item_size;
  return capacity;
}

/////////////////////////////////////////////////////////////////////////////////
IndexNodeHandler::IndexNodeHandler(const IndexFileHeader &header, Frame *frame)
  : header_(header), page_num_(frame->page_num()), node_((IndexNode *)frame->data())
{}

bool IndexNodeHandler::is_leaf() const
{
  return node_->is_leaf;
}
void IndexNodeHandler::init_empty(bool leaf)
{
  node_->is_leaf = leaf;
  node_->key_num = 0;
  node_->parent = BP_INVALID_PAGE_NUM;
}
PageNum IndexNodeHandler::page_num() const
{
  return page_num_;
}

int IndexNodeHandler::key_size() const
{
  return header_.key_length;
}

int IndexNodeHandler::value_size() const
{
  // return header_.value_size;
  return sizeof(RID);
}

int IndexNodeHandler::item_size() const
{
  return key_size() + value_size();
}

int IndexNodeHandler::size() const
{
  return node_->key_num;
}

void IndexNodeHandler::increase_size(int n)
{
  node_->key_num += n;
}

PageNum IndexNodeHandler::parent_page_num() const
{
  return node_->parent;
}

void IndexNodeHandler::set_parent_page_num(PageNum page_num)
{
  this->node_->parent = page_num;
}
std::string to_string(const IndexNodeHandler &handler)
{
  std::stringstream ss;

  ss << "PageNum:" << handler.page_num()
     << ",is_leaf:" << handler.is_leaf() << ","
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
  leaf_node_->prev_brother = BP_INVALID_PAGE_NUM;
  leaf_node_->next_brother = BP_INVALID_PAGE_NUM;
}

void LeafIndexNodeHandler::set_next_page(PageNum page_num)
{
  leaf_node_->next_brother = page_num;
}

void LeafIndexNodeHandler::set_prev_page(PageNum page_num)
{
  leaf_node_->prev_brother = page_num;
}
PageNum LeafIndexNodeHandler::next_page() const
{
  return leaf_node_->next_brother;
}
PageNum LeafIndexNodeHandler::prev_page() const
{
  return leaf_node_->prev_brother;
}

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

int LeafIndexNodeHandler::max_size() const
{
  return header_.leaf_max_size;
}

int LeafIndexNodeHandler::min_size() const
{
  return header_.leaf_max_size - header_.leaf_max_size / 2;
}

int LeafIndexNodeHandler::lookup(const KeyComparator &comparator, const char *key, bool *found /* = nullptr */) const
{
  const int size = this->size();
  common::BinaryIterator<char> iter_begin(item_size(), __key_at(0));
  common::BinaryIterator<char> iter_end(item_size(), __key_at(size));
  common::BinaryIterator<char> iter = lower_bound(iter_begin, iter_end, key, comparator, found);
  return iter - iter_begin;
}

void LeafIndexNodeHandler::insert(int index, const char *key, const char *value)
{
  if (index < size()) {
    memmove(__item_at(index + 1), __item_at(index), (size() - index) * item_size());
  }
  memcpy(__item_at(index), key, key_size());
  memcpy(__item_at(index) + key_size(), value, value_size());
  increase_size(1);
}
void LeafIndexNodeHandler::remove(int index)
{
  assert(index >= 0 && index < size());
  if (index < size() - 1) {
    memmove(__item_at(index), __item_at(index + 1), (size() - index - 1) * item_size());
  }
  increase_size(-1);
}

int LeafIndexNodeHandler::remove(const char *key, const KeyComparator &comparator)
{
  bool found = false;
  int index = lookup(comparator, key, &found);
  if (found) {
    this->remove(index);
    return 1;
  }
  return 0;
}

RC LeafIndexNodeHandler::move_half_to(LeafIndexNodeHandler &other, DiskBufferPool *bp)
{
  const int size = this->size();
  const int move_index = size / 2;

  memcpy(other.__item_at(0), this->__item_at(move_index), item_size() * (size - move_index));
  other.increase_size(size - move_index);
  this->increase_size(- ( size - move_index));
  return RC::SUCCESS;
}
RC LeafIndexNodeHandler::move_first_to_end(LeafIndexNodeHandler &other, DiskBufferPool *disk_buffer_pool)
{
  other.append(__item_at(0));

  if (size() >= 1) {
    memmove(__item_at(0), __item_at(1), (size() - 1) * item_size() );
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
  memcpy(other.__item_at(other.size()), this->__item_at(0), this->size() * item_size());
  other.increase_size(this->size());
  this->increase_size(- this->size());

  other.set_next_page(this->next_page());

  PageNum next_right_page_num = this->next_page();
  if (next_right_page_num != BP_INVALID_PAGE_NUM) {
    Frame *next_right_frame;
    RC rc = bp->get_this_page(next_right_page_num, &next_right_frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch next right page. page number:%d. rc=%d:%s", next_right_page_num, rc, strrc(rc));
      return rc;
    }

    LeafIndexNodeHandler next_right_node(header_, next_right_frame);
    next_right_node.set_prev_page(other.page_num());
    next_right_frame->mark_dirty();
    bp->unpin_page(next_right_frame);
  }
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
    memmove(__item_at(1), __item_at(0), size() * item_size());
  }
  memcpy(__item_at(0), item, item_size());
  increase_size(1);
}

char *LeafIndexNodeHandler::__item_at(int index) const
{
  return leaf_node_->array + (index * item_size());
}
char *LeafIndexNodeHandler::__key_at(int index) const
{
  return __item_at(index);
}
char *LeafIndexNodeHandler::__value_at(int index) const
{
  return __item_at(index) + key_size();
}

std::string to_string(const LeafIndexNodeHandler &handler, const KeyPrinter &printer)
{
  std::stringstream ss;
  ss << to_string((const IndexNodeHandler &)handler)
     << ",prev page:" << handler.prev_page()
     << ",next page:" << handler.next_page();
  ss << ",values=[" << printer(handler.__key_at(0)) ;
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
	       page_num(), i-1, i, to_string(*this).c_str());
      return false;
    }
  }

  PageNum parent_page_num = this->parent_page_num();
  if (parent_page_num == BP_INVALID_PAGE_NUM) {
    return true;
  }

  Frame *parent_frame;
  RC rc = bp->get_this_page(parent_page_num, &parent_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch parent page. page num=%d, rc=%d:%s",
	     parent_page_num, rc, strrc(rc));
    return false;
  }

  InternalIndexNodeHandler parent_node(header_, parent_frame);
  int index_in_parent = parent_node.value_index(this->page_num());
  if (index_in_parent < 0) {
    LOG_WARN("invalid leaf node. cannot find index in parent. this page num=%d, parent page num=%d",
	     this->page_num(), parent_page_num);
    bp->unpin_page(parent_frame);
    return false;
  }

  if (0 != index_in_parent) {
    int cmp_result = comparator(__key_at(0), parent_node.key_at(index_in_parent));
    if (cmp_result < 0) {
      LOG_WARN("invalid leaf node. first item should be greate than or equal to parent item. " \
	       "this page num=%d, parent page num=%d, index in parent=%d",
	       this->page_num(), parent_node.page_num(), index_in_parent);
      bp->unpin_page(parent_frame);
      return false;
    }
  }

  if (index_in_parent < parent_node.size() - 1) {
    int cmp_result = comparator(__key_at(size() - 1), parent_node.key_at(index_in_parent + 1));
    if (cmp_result >= 0) {
      LOG_WARN("invalid leaf node. last item should be less than the item at the first after item in parent." \
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
    ss << ",{key:" << printer(node.__key_at(i))
       << ",value:"<< *(PageNum *)node.__value_at(i) << "}";
  }
  ss << "]";
  return ss.str();
}

void InternalIndexNodeHandler::init_empty()
{
  IndexNodeHandler::init_empty(false);
}
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
    memmove(__item_at(insert_position + 1), __item_at(insert_position), (size() - insert_position) * item_size());
  }
  memcpy(__item_at(insert_position), key, key_size());
  memcpy(__value_at(insert_position), &page_num, value_size());
  increase_size(1);
}

RC InternalIndexNodeHandler::move_half_to(InternalIndexNodeHandler &other, DiskBufferPool *bp)
{
  const int size = this->size();
  const int move_index = size / 2;
  RC rc = other.copy_from(this->__item_at(move_index), size - move_index, bp);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to copy item to new node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  increase_size(- (size - move_index));
  return rc;
}

int InternalIndexNodeHandler::max_size() const
{
  return header_.internal_max_size;
}

int InternalIndexNodeHandler::min_size() const
{
  return header_.internal_max_size - header_.internal_max_size / 2;
}

/**
 * lookup the first item which key <= item
 * @return unlike the leafNode, the return value is not the insert position,
 * but only the index of child to find. 
 */
int InternalIndexNodeHandler::lookup(const KeyComparator &comparator, const char *key,
				     bool *found /* = nullptr */, int *insert_position /*= nullptr */) const
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
  int ret = static_cast<int>(iter - iter_begin) + 1;
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
    if (page_num == *(PageNum*)__value_at(i)) {
      return i;
    }
  }
  return -1;
}

void InternalIndexNodeHandler::remove(int index)
{
  assert(index >= 0 && index < size());
  if (index < size() - 1) {
    memmove(__item_at(index), __item_at(index + 1), (size() - index - 1) * item_size());
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

  increase_size(- this->size());
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
    memmove(__item_at(0), __item_at(1), (size() - 1) * item_size() );
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
  memcpy(__item_at(this->size()), items, num * item_size());

  RC rc = RC::SUCCESS;
  PageNum this_page_num = this->page_num();
  Frame *frame = nullptr;
  for (int i = 0; i < num; i++) {
    const PageNum page_num = *(const PageNum *)((items + i * item_size()) + key_size());
    rc = disk_buffer_pool->get_this_page(page_num, &frame);
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

RC InternalIndexNodeHandler::append(const char *item, DiskBufferPool *bp)
{
  return this->copy_from(item, 1, bp);
}

RC InternalIndexNodeHandler::preappend(const char *item, DiskBufferPool *bp)
{
  PageNum child_page_num = *(PageNum *)(item + key_size());
  Frame *frame = nullptr;
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
    memmove(__item_at(1), __item_at(0), this->size() * item_size());
  }

  memcpy(__item_at(0), item, item_size());
  increase_size(1);
  return RC::SUCCESS;
}

char *InternalIndexNodeHandler::__item_at(int index) const
{
  return internal_node_->array + (index * item_size());
}

char *InternalIndexNodeHandler::__key_at(int index) const
{
  return __item_at(index);
}

char *InternalIndexNodeHandler::__value_at(int index) const
{
  return __item_at(index) + key_size();
}

int InternalIndexNodeHandler::value_size() const
{
  return sizeof(PageNum);
}

int InternalIndexNodeHandler::item_size() const
{
  return key_size() + this->value_size();
}

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
	       page_num(), i-1, i, to_string(*this).c_str());
      return false;
    }
  }

  for (int i = 0; result && i < node_size; i++) {
    PageNum page_num = *(PageNum *)__value_at(i);
    if (page_num < 0) {
      LOG_WARN("this page num=%d, got invalid child page. page num=%d", this->page_num(), page_num);
    } else {
      Frame *child_frame;
      RC rc = bp->get_this_page(page_num, &child_frame);
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
  RC rc = bp->get_this_page(parent_page_num, &parent_frame);
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
      LOG_WARN("invalid internal node. the second item should be greate than or equal to parent item. " \
	       "this page num=%d, parent page num=%d, index in parent=%d",
	       this->page_num(), parent_node.page_num(), index_in_parent);
      bp->unpin_page(parent_frame);
      return false;
    }
  }

  if (index_in_parent < parent_node.size() - 1) {
    int cmp_result = comparator(__key_at(size() - 1), parent_node.key_at(index_in_parent + 1));
    if (cmp_result >= 0) {
      LOG_WARN("invalid internal node. last item should be less than the item at the first after item in parent." \
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
  return disk_buffer_pool_->flush_all_pages();
}

RC BplusTreeHandler::create(const char *file_name, AttrType attr_type, int attr_length,
			    int internal_max_size /* = -1*/, int leaf_max_size /* = -1 */)
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

  char *pdata = header_frame->data();
  IndexFileHeader *file_header = (IndexFileHeader *)pdata;
  file_header->attr_length = attr_length;
  file_header->key_length = attr_length + sizeof(RID);
  file_header->attr_type = attr_type;
  file_header->internal_max_size = internal_max_size;
  file_header->leaf_max_size = leaf_max_size;
  file_header->root_page = BP_INVALID_PAGE_NUM;

  header_frame->mark_dirty();

  disk_buffer_pool_ = bp;

  memcpy(&file_header_, pdata, sizeof(file_header_));
  header_dirty_ = false;
  bp->unpin_page(header_frame);

  mem_pool_item_ = new common::MemPoolItem(file_name);
  if (mem_pool_item_->init(file_header->key_length) < 0) {
    LOG_WARN("Failed to init memory pool for index %s", file_name);
    close();
    return RC::NOMEM;
  }

  key_comparator_.init(file_header->attr_type, file_header->attr_length);
  key_printer_.init(file_header->attr_type, file_header->attr_length);
  LOG_INFO("Successfully create index %s", file_name);
  return RC::SUCCESS;
}

RC BplusTreeHandler::open(const char *file_name)
{
  if (disk_buffer_pool_ != nullptr) {
    LOG_WARN("%s has been opened before index.open.", file_name);
    return RC::RECORD_OPENNED;
  }

  BufferPoolManager &bpm = BufferPoolManager::instance();
  DiskBufferPool *disk_buffer_pool;
  RC rc = bpm.open_file(file_name, disk_buffer_pool);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to open file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    return rc;
  }

  Frame *frame;
  rc = disk_buffer_pool->get_this_page(FIRST_INDEX_PAGE, &frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get first page file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    bpm.close_file(file_name);
    return rc;
  }

  char *pdata = frame->data();
  memcpy(&file_header_, pdata, sizeof(IndexFileHeader));
  header_dirty_ = false;
  disk_buffer_pool_ = disk_buffer_pool;

  mem_pool_item_ = new common::MemPoolItem(file_name);
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

    disk_buffer_pool_->close_file(); // TODO

    delete mem_pool_item_;
    mem_pool_item_ = nullptr;
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
    Frame *child_frame;
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

  Frame *frame;
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

  Frame *frame;
  
  RC rc = left_most_page(frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get left most page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  while (frame->page_num() != BP_INVALID_PAGE_NUM) {
    LeafIndexNodeHandler leaf_node(file_header_, frame);
    LOG_INFO("leaf info: %s", to_string(leaf_node, key_printer_).c_str());

    PageNum next_page_num = leaf_node.next_page();
    disk_buffer_pool_->unpin_page(frame);

    if (next_page_num == BP_INVALID_PAGE_NUM) {
      break;
    }

    rc = disk_buffer_pool_->get_this_page(next_page_num, &frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get next page. page id=%d, rc=%d:%s", next_page_num, rc, strrc(rc));
      return rc;
    }
  }
  return rc;
}

bool BplusTreeHandler::validate_node_recursive(Frame *frame)
{
  bool result = true;
  IndexNodeHandler node(file_header_, frame);
  if (node.is_leaf()) {
    LeafIndexNodeHandler leaf_node(file_header_, frame);
    result = leaf_node.validate(key_comparator_, disk_buffer_pool_);
  } else {
    InternalIndexNodeHandler internal_node(file_header_, frame);
    result = internal_node.validate(key_comparator_, disk_buffer_pool_);
    for (int i = 0; result && i < internal_node.size(); i++) {
      PageNum page_num = internal_node.value_at(i);
      Frame *child_frame;
      RC rc = disk_buffer_pool_->get_this_page(page_num, &child_frame);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to fetch child page.page id=%d, rc=%d:%s", page_num, rc, strrc(rc));
        result = false;
        break;
      }

      result = validate_node_recursive(child_frame);
    }
  }

  disk_buffer_pool_->unpin_page(frame);
  return result;
}

bool BplusTreeHandler::validate_leaf_link()
{
  if (is_empty()) {
    return true;
  }

  Frame *frame;
  RC rc = left_most_page(frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch left most page. rc=%d:%s", rc, strrc(rc));
    return false;
  }

  PageNum prev_page_num = BP_INVALID_PAGE_NUM;

  LeafIndexNodeHandler leaf_node(file_header_, frame);
  if (leaf_node.prev_page() != prev_page_num) {
    LOG_WARN("invalid page. current_page_num=%d, prev page num should be %d but got %d",
  	       frame->page_num(), prev_page_num, leaf_node.prev_page());
    return false;
  }
  PageNum next_page_num = leaf_node.next_page();

  prev_page_num = frame->page_num();
  char *prev_key = (char *)mem_pool_item_->alloc();
  memcpy(prev_key, leaf_node.key_at(leaf_node.size() - 1), file_header_.key_length);
  disk_buffer_pool_->unpin_page(frame);

  bool result = true;
  while (result && next_page_num != BP_INVALID_PAGE_NUM) {
    rc = disk_buffer_pool_->get_this_page(next_page_num, &frame);
    if (rc != RC::SUCCESS) {
      free_key(prev_key);
      LOG_WARN("failed to fetch next page. page num=%d, rc=%d:%s", next_page_num, rc, strrc(rc));
      return false;
    }

    LeafIndexNodeHandler leaf_node(file_header_, frame);
    if (leaf_node.prev_page() != prev_page_num) {
      LOG_WARN("invalid page. current_page_num=%d, prev page num should be %d but got %d",
	       frame->page_num(), prev_page_num, leaf_node.prev_page());
      result = false;
    }
    if (key_comparator_(prev_key, leaf_node.key_at(0)) >= 0) {
      LOG_WARN("invalid page. current first key is not bigger than last");
      result = false;
    }

    next_page_num = leaf_node.next_page();
    memcpy(prev_key, leaf_node.key_at(leaf_node.size() - 1), file_header_.key_length);
    prev_page_num = frame->page_num();
    disk_buffer_pool_->unpin_page(frame);
  }

  free_key(prev_key);
  // can do more things
  return result;
}

bool BplusTreeHandler::validate_tree()
{
  if (is_empty()) {
    return true;
  }

  Frame *frame;
  RC rc = disk_buffer_pool_->get_this_page(file_header_.root_page, &frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch root page. page id=%d, rc=%d:%s", file_header_.root_page, rc, strrc(rc));
    return rc;
  }

  if (!validate_node_recursive(frame) || !validate_leaf_link()) {
    LOG_WARN("Current B+ Tree is invalid");
    print_tree();
    return false;
  }

  LOG_INFO("great! current tree is valid");
  return true;
}

bool BplusTreeHandler::is_empty() const
{
  return file_header_.root_page == BP_INVALID_PAGE_NUM;
}

RC BplusTreeHandler::find_leaf(const char *key, Frame *&frame)
{
  return find_leaf_internal(
			    [&](InternalIndexNodeHandler &internal_node) {
			      return internal_node.value_at(internal_node.lookup(key_comparator_, key));
			    },
			    frame);
}

RC BplusTreeHandler::left_most_page(Frame *&frame)
{
  return find_leaf_internal(
			    [&](InternalIndexNodeHandler &internal_node) {
			      return internal_node.value_at(0);
			    },
			    frame
			    );
}
RC BplusTreeHandler::right_most_page(Frame *&frame)
{
  return find_leaf_internal(
			    [&](InternalIndexNodeHandler &internal_node) {
			      return internal_node.value_at(internal_node.size() - 1);
			    },
			    frame
			    );
}

RC BplusTreeHandler::find_leaf_internal(const std::function<PageNum(InternalIndexNodeHandler &)> &child_page_getter,
					Frame *&frame)
{
  if (is_empty()) {
    return RC::EMPTY;
  }

  RC rc = disk_buffer_pool_->get_this_page(file_header_.root_page, &frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch root page. page id=%d, rc=%d:%s", file_header_.root_page, rc, strrc(rc));
    return rc;
  }

  IndexNode *node = (IndexNode *)frame->data();
  while (false == node->is_leaf) {
    InternalIndexNodeHandler internal_node(file_header_, frame);
    PageNum page_num = child_page_getter(internal_node);

    disk_buffer_pool_->unpin_page(frame);

    rc = disk_buffer_pool_->get_this_page(page_num, &frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to load page page_num:%d", page_num);
      return rc;
    }
    node = (IndexNode *)frame->data();
  }

  return RC::SUCCESS;
}

RC BplusTreeHandler::insert_entry_into_leaf_node(Frame *frame, const char *key, const RID *rid)
{
  LeafIndexNodeHandler leaf_node(file_header_, frame);
  bool exists = false;
  int insert_position = leaf_node.lookup(key_comparator_, key, &exists);
  if (exists) {
    LOG_TRACE("entry exists");
    return RC::RECORD_DUPLICATE_KEY;
  }

  if (leaf_node.size() < leaf_node.max_size()) {
    leaf_node.insert(insert_position, key, (const char *)rid);
    frame->mark_dirty();
    disk_buffer_pool_->unpin_page(frame);
    return RC::SUCCESS;
  }

  Frame * new_frame = nullptr;
  RC rc = split<LeafIndexNodeHandler>(frame, new_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to split leaf node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  LeafIndexNodeHandler new_index_node(file_header_, new_frame);
  new_index_node.set_prev_page(frame->page_num());
  new_index_node.set_next_page(leaf_node.next_page());
  new_index_node.set_parent_page_num(leaf_node.parent_page_num());
  leaf_node.set_next_page(new_frame->page_num());

  PageNum next_page_num = new_index_node.next_page();
  if (next_page_num != BP_INVALID_PAGE_NUM) {
    Frame * next_frame;
    rc = disk_buffer_pool_->get_this_page(next_page_num, &next_frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch next page. page num=%d, rc=%d:%s", next_page_num, rc, strrc(rc));
      return rc;
    }

    LeafIndexNodeHandler next_node(file_header_, next_frame);
    next_node.set_prev_page(new_frame->page_num());
    disk_buffer_pool_->unpin_page(next_frame);
  }

  if (insert_position < leaf_node.size()) {
    leaf_node.insert(insert_position, key, (const char *)rid);
  } else {
    new_index_node.insert(insert_position - leaf_node.size(), key, (const char *)rid);
  }

  return insert_entry_into_parent(frame, new_frame, new_index_node.key_at(0));
}

RC BplusTreeHandler::insert_entry_into_parent(Frame *frame, Frame *new_frame, const char *key)
{
  RC rc = RC::SUCCESS;

  IndexNodeHandler node_handler(file_header_, frame);
  IndexNodeHandler new_node_handler(file_header_, new_frame);
  PageNum parent_page_num = node_handler.parent_page_num();

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
    disk_buffer_pool_->unpin_page(frame);
    disk_buffer_pool_->unpin_page(new_frame);

    file_header_.root_page = root_frame->page_num();
    update_root_page_num(); // TODO
    root_frame->mark_dirty();
    disk_buffer_pool_->unpin_page(root_frame);

    return RC::SUCCESS;

  } else {

    Frame *parent_frame;
    rc = disk_buffer_pool_->get_this_page(parent_page_num, &parent_frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to insert entry into leaf. rc=%d:%s", rc, strrc(rc));
      // should do more things to recover
      return rc;
    }

    InternalIndexNodeHandler node(file_header_, parent_frame);

    /// current node is not in full mode, insert the entry and return
    if (node.size() < node.max_size()) {
      node.insert(key, new_frame->page_num(), key_comparator_);
      new_node_handler.set_parent_page_num(parent_page_num);

      frame->mark_dirty();
      new_frame->mark_dirty();
      parent_frame->mark_dirty();
      disk_buffer_pool_->unpin_page(frame);
      disk_buffer_pool_->unpin_page(new_frame);
      disk_buffer_pool_->unpin_page(parent_frame);

    } else {

      // we should split the node and insert the entry and then insert new entry to current node's parent
      Frame * new_parent_frame;
      rc = split<InternalIndexNodeHandler>(parent_frame, new_parent_frame);
      if (rc != RC::SUCCESS) {
	LOG_WARN("failed to split internal node. rc=%d:%s", rc, strrc(rc));
	disk_buffer_pool_->unpin_page(frame);
	disk_buffer_pool_->unpin_page(new_frame);
	disk_buffer_pool_->unpin_page(parent_frame);
      } else {
	// insert into left or right ? decide by key compare result
	InternalIndexNodeHandler new_node(file_header_, new_parent_frame);
	if (key_comparator_(key, new_node.key_at(0)) > 0) {
	  new_node.insert(key, new_frame->page_num(), key_comparator_);
          new_node_handler.set_parent_page_num(new_node.page_num());
	} else {
	  node.insert(key, new_frame->page_num(), key_comparator_);
          new_node_handler.set_parent_page_num(node.page_num());
	}

	disk_buffer_pool_->unpin_page(frame);
	disk_buffer_pool_->unpin_page(new_frame);
	
	rc = insert_entry_into_parent(parent_frame, new_parent_frame, new_node.key_at(0));
      }
    }
  }
  return rc;
}

/**
 * split one full node into two
 * @param page_handle[inout] the node to split
 * @param new_page_handle[out] the new node after split
 * @param intert_position the intert position of new key
 */
template <typename IndexNodeHandlerType>
RC BplusTreeHandler::split(Frame *frame, Frame *&new_frame)
{
  IndexNodeHandlerType old_node(file_header_, frame);

  // add a new node
  RC rc = disk_buffer_pool_->allocate_page(&new_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to split index page due to failed to allocate page, rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  IndexNodeHandlerType new_node(file_header_, new_frame);
  new_node.init_empty();
  new_node.set_parent_page_num(old_node.parent_page_num());

  old_node.move_half_to(new_node, disk_buffer_pool_);

  frame->mark_dirty();
  new_frame->mark_dirty();
  return RC::SUCCESS;
}

RC BplusTreeHandler::update_root_page_num()
{
  Frame * header_frame;
  RC rc = disk_buffer_pool_->get_this_page(FIRST_INDEX_PAGE, &header_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch header page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  IndexFileHeader *header = (IndexFileHeader *)header_frame->data();
  header->root_page = file_header_.root_page;
  header_frame->mark_dirty();
  disk_buffer_pool_->unpin_page(header_frame);
  return rc;
}


RC BplusTreeHandler::create_new_tree(const char *key, const RID *rid)
{
  RC rc = RC::SUCCESS;
  if (file_header_.root_page != BP_INVALID_PAGE_NUM) {
    rc = RC::INTERNAL;
    LOG_WARN("cannot create new tree while root page is valid. root page id=%d", file_header_.root_page);
    return rc;
  }

  Frame *frame;
  rc = disk_buffer_pool_->allocate_page(&frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to allocate root page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  LeafIndexNodeHandler leaf_node(file_header_, frame);
  leaf_node.init_empty();
  leaf_node.insert(0, key, (const char *)rid);
  file_header_.root_page = frame->page_num();
  frame->mark_dirty();
  disk_buffer_pool_->unpin_page(frame);

  rc = update_root_page_num();
  // disk_buffer_pool_->check_all_pages_unpinned(file_id_);
  return rc;
}

char *BplusTreeHandler::make_key(const char *user_key, const RID &rid)
{
  char *key = (char *)mem_pool_item_->alloc();
  if (key == nullptr) {
    LOG_WARN("Failed to alloc memory for key.");
    return nullptr;
  }
  memcpy(key, user_key, file_header_.attr_length);
  memcpy(key + file_header_.attr_length, &rid, sizeof(rid));
  return key;
}

void BplusTreeHandler::free_key(char *key)
{
  mem_pool_item_->free(key);
}

RC BplusTreeHandler::insert_entry(const char *user_key, const RID *rid)
{
  if (user_key == nullptr || rid == nullptr) {
    LOG_WARN("Invalid arguments, key is empty or rid is empty");
    return RC::INVALID_ARGUMENT;
  }

  char *key = make_key(user_key, *rid);
  if (key == nullptr) {
    LOG_WARN("Failed to alloc memory for key.");
    return RC::NOMEM;
  }

  if (is_empty()) {
    RC rc = create_new_tree(key, rid);
    mem_pool_item_->free(key);
    return rc;
  }

  Frame *frame;
  RC rc = find_leaf(key, frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to find leaf %s. rc=%d:%s", rid->to_string().c_str(), rc, strrc(rc));
    mem_pool_item_->free(key);
    return rc;
  }

  rc = insert_entry_into_leaf_node(frame, key, rid);
  if (rc != RC::SUCCESS) {
    LOG_TRACE("Failed to insert into leaf of index, rid:%s", rid->to_string().c_str());
    disk_buffer_pool_->unpin_page(frame);
    mem_pool_item_->free(key);
    // disk_buffer_pool_->check_all_pages_unpinned(file_id_);
    return rc;
  }

  mem_pool_item_->free(key);
  LOG_TRACE("insert entry success");
  // disk_buffer_pool_->check_all_pages_unpinned(file_id_);
  return RC::SUCCESS;
}

RC BplusTreeHandler::get_entry(const char *user_key, int key_len, std::list<RID> &rids)
{
  BplusTreeScanner scanner(*this);
  RC rc = scanner.open(user_key, key_len, true/*left_inclusive*/, user_key, key_len, true/*right_inclusive*/);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open scanner. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  RID rid;
  while ((rc = scanner.next_entry(&rid)) == RC::SUCCESS) {
    rids.push_back(rid);
  }

  scanner.close();
  if (rc != RC::RECORD_EOF) {
    LOG_WARN("scanner return error. rc=%d:%s", rc, strrc(rc));
  } else {
    rc = RC::SUCCESS;
  }
  return rc;
}

RC BplusTreeHandler::adjust_root(Frame *root_frame)
{
  IndexNodeHandler root_node(file_header_, root_frame);
  if (root_node.is_leaf() && root_node.size() > 0) {
    root_frame->mark_dirty();
    disk_buffer_pool_->unpin_page(root_frame);
    return RC::SUCCESS;
  }

  if (root_node.is_leaf()) {
    // this is a leaf and an empty node
    file_header_.root_page = BP_INVALID_PAGE_NUM;
  } else {
    // this is an internal node and has only one child node
    InternalIndexNodeHandler internal_node(file_header_, root_frame);

    const PageNum child_page_num = internal_node.value_at(0);
    Frame * child_frame;
    RC rc = disk_buffer_pool_->get_this_page(child_page_num, &child_frame);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch child page. page num=%d, rc=%d:%s", child_page_num, rc, strrc(rc));
      return rc;
    }

    IndexNodeHandler child_node(file_header_, child_frame);
    child_node.set_parent_page_num(BP_INVALID_PAGE_NUM);
    disk_buffer_pool_->unpin_page(child_frame);
    
    file_header_.root_page = child_page_num;
  }

  update_root_page_num();

  PageNum old_root_page_num = root_frame->page_num();
  disk_buffer_pool_->unpin_page(root_frame);
  disk_buffer_pool_->dispose_page(old_root_page_num);
  return RC::SUCCESS;
}
template <typename IndexNodeHandlerType>
RC BplusTreeHandler::coalesce_or_redistribute(Frame *frame)
{
  IndexNodeHandlerType index_node(file_header_, frame);
  if (index_node.size() >= index_node.min_size()) {
    disk_buffer_pool_->unpin_page(frame);
    return RC::SUCCESS;
  }

  const PageNum parent_page_num = index_node.parent_page_num();
  if (BP_INVALID_PAGE_NUM == parent_page_num) {
    // this is the root page
    if (index_node.size() > 1) {
      disk_buffer_pool_->unpin_page(frame);
    } else {
      // adjust the root node
      adjust_root(frame);
    }
    return RC::SUCCESS;
  }

  Frame * parent_frame;
  RC rc = disk_buffer_pool_->get_this_page(parent_page_num, &parent_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch parent page. page id=%d, rc=%d:%s", parent_page_num, rc, strrc(rc));
    disk_buffer_pool_->unpin_page(frame);
    return rc;
  }

  InternalIndexNodeHandler parent_index_node(file_header_, parent_frame);
  int index = parent_index_node.lookup(key_comparator_, index_node.key_at(index_node.size() - 1));
  if (parent_index_node.value_at(index) != frame->page_num()) {
    LOG_ERROR("lookup return an invalid value. index=%d, this page num=%d, but got %d",
	      index, frame->page_num(), parent_index_node.value_at(index));
  }
  PageNum neighbor_page_num;
  if (index == 0) {
    neighbor_page_num = parent_index_node.value_at(1);
  } else {
    neighbor_page_num = parent_index_node.value_at(index - 1);
  }

  Frame * neighbor_frame;
  rc = disk_buffer_pool_->get_this_page(neighbor_page_num, &neighbor_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch neighbor page. page id=%d, rc=%d:%s", neighbor_page_num, rc, strrc(rc));
    // TODO do more thing to release resource
    disk_buffer_pool_->unpin_page(frame);
    disk_buffer_pool_->unpin_page(parent_frame);
    return rc;
  }

  IndexNodeHandlerType neighbor_node(file_header_, neighbor_frame);
  if (index_node.size() + neighbor_node.size() > index_node.max_size()) {
    rc = redistribute<IndexNodeHandlerType>(neighbor_frame, frame, parent_frame, index);
  } else {
    rc = coalesce<IndexNodeHandlerType>(neighbor_frame, frame, parent_frame, index);
  }
  return rc;
}

template <typename IndexNodeHandlerType>
RC BplusTreeHandler::coalesce(Frame *neighbor_frame, Frame *frame, Frame *parent_frame, int index)
{
  IndexNodeHandlerType neighbor_node(file_header_, neighbor_frame);
  IndexNodeHandlerType node(file_header_, frame);

  InternalIndexNodeHandler parent_node(file_header_, parent_frame);

  Frame *left_frame = nullptr;
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

    PageNum next_right_page_num = right_leaf_node.next_page();
    if (next_right_page_num != BP_INVALID_PAGE_NUM) {
      Frame *next_right_frame;
      rc = disk_buffer_pool_->get_this_page(next_right_page_num, &next_right_frame);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to fetch next right page. page number:%d. rc=%d:%s", next_right_page_num, rc, strrc(rc));
	disk_buffer_pool_->unpin_page(frame);
	disk_buffer_pool_->unpin_page(neighbor_frame);
	disk_buffer_pool_->unpin_page(parent_frame);
        return rc;
      }

      LeafIndexNodeHandler next_right_node(file_header_, next_right_frame);
      next_right_node.set_prev_page(left_node.page_num());
      disk_buffer_pool_->unpin_page(next_right_frame);
    }
    
  }

  PageNum right_page_num = right_frame->page_num();
  disk_buffer_pool_->unpin_page(left_frame);
  disk_buffer_pool_->unpin_page(right_frame);
  disk_buffer_pool_->dispose_page(right_page_num);
  return coalesce_or_redistribute<InternalIndexNodeHandler>(parent_frame);
}

template <typename IndexNodeHandlerType>
RC BplusTreeHandler::redistribute(Frame *neighbor_frame, Frame *frame, Frame *parent_frame, int index)
{
  InternalIndexNodeHandler parent_node(file_header_, parent_frame);
  IndexNodeHandlerType neighbor_node(file_header_, neighbor_frame);
  IndexNodeHandlerType node(file_header_, frame);
  if (neighbor_node.size() < node.size()) {
    LOG_ERROR("got invalid nodes. neighbor node size %d, this node size %d",
	      neighbor_node.size(), node.size());
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
  disk_buffer_pool_->unpin_page(parent_frame);
  disk_buffer_pool_->unpin_page(neighbor_frame);
  disk_buffer_pool_->unpin_page(frame);
  return RC::SUCCESS;
}

RC BplusTreeHandler::delete_entry_internal(Frame *leaf_frame, const char *key)
{
  LeafIndexNodeHandler leaf_index_node(file_header_, leaf_frame);

  const int remove_count = leaf_index_node.remove(key, key_comparator_);
  if (remove_count == 0) {
    LOG_TRACE("no data to remove");
    disk_buffer_pool_->unpin_page(leaf_frame);
    return RC::RECORD_RECORD_NOT_EXIST;
  }
  // leaf_index_node.validate(key_comparator_, disk_buffer_pool_, file_id_);

  leaf_frame->mark_dirty();

  if (leaf_index_node.size() >= leaf_index_node.min_size()) {
    disk_buffer_pool_->unpin_page(leaf_frame);
    return RC::SUCCESS;
  }

  return coalesce_or_redistribute<LeafIndexNodeHandler>(leaf_frame);
}

RC BplusTreeHandler::delete_entry(const char *user_key, const RID *rid)
{
  char *key = (char *)mem_pool_item_->alloc();
  if (nullptr == key) {
    LOG_WARN("Failed to alloc memory for key. size=%d", file_header_.key_length);
    return RC::NOMEM;
  }
  memcpy(key, user_key, file_header_.attr_length);
  memcpy(key + file_header_.attr_length, rid, sizeof(*rid));

  Frame *leaf_frame;
  RC rc = find_leaf(key, leaf_frame);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to find leaf page. rc =%d:%s", rc, strrc(rc));
    mem_pool_item_->free(key);
    return rc;
  }
  rc = delete_entry_internal(leaf_frame, key);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to delete index");
    mem_pool_item_->free(key);
    return rc;
  }
  mem_pool_item_->free(key);
  return RC::SUCCESS;
}

BplusTreeScanner::BplusTreeScanner(BplusTreeHandler &tree_handler) : tree_handler_(tree_handler)
{}

BplusTreeScanner::~BplusTreeScanner()
{
  close();
}

RC BplusTreeScanner::open(const char *left_user_key, int left_len, bool left_inclusive,
                          const char *right_user_key, int right_len, bool right_inclusive)
{
  RC rc = RC::SUCCESS;
  if (inited_) {
    LOG_WARN("tree scanner has been inited");
    return RC::INTERNAL;
  }

  inited_ = true;
  
  // 校验输入的键值是否是合法范围
  if (left_user_key && right_user_key) {
    const auto &attr_comparator = tree_handler_.key_comparator_.attr_comparator();
    const int result = attr_comparator(left_user_key, right_user_key);
    if (result > 0 || // left < right
         // left == right but is (left,right)/[left,right) or (left,right]
	(result == 0 && (left_inclusive == false || right_inclusive == false))) { 
      return RC::INVALID_ARGUMENT;
    }
  }

  if (nullptr == left_user_key) {
    rc = tree_handler_.left_most_page(left_frame_);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to find left most page. rc=%d:%s", rc, strrc(rc));
      return rc;
    }

    iter_index_ = 0;
  } else {
    char *left_key = nullptr;

    char *fixed_left_key = const_cast<char *>(left_user_key);
    if (tree_handler_.file_header_.attr_type == CHARS) {
      bool should_inclusive_after_fix = false;
      rc = fix_user_key(left_user_key, left_len, true/*greater*/, &fixed_left_key, &should_inclusive_after_fix);
      if (rc != RC::SUCCESS) {
	LOG_WARN("failed to fix left user key. rc=%s", strrc(rc));
	return rc;
      }

      if (should_inclusive_after_fix) {
	left_inclusive = true;
      }
    }

    if (left_inclusive) {
      left_key = tree_handler_.make_key(fixed_left_key, *RID::min());
    } else {
      left_key = tree_handler_.make_key(fixed_left_key, *RID::max());
    }

    if (fixed_left_key != left_user_key) {
      delete[] fixed_left_key;
      fixed_left_key = nullptr;
    }
    rc = tree_handler_.find_leaf(left_key, left_frame_);

    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to find left page. rc=%d:%s", rc, strrc(rc));
      tree_handler_.free_key(left_key);
      return rc;
    }
    LeafIndexNodeHandler left_node(tree_handler_.file_header_, left_frame_);
    int left_index = left_node.lookup(tree_handler_.key_comparator_, left_key);
    tree_handler_.free_key(left_key);
    // lookup 返回的是适合插入的位置，还需要判断一下是否在合适的边界范围内
    if (left_index >= left_node.size()) { // 超出了当前页，就需要向后移动一个位置
      const PageNum next_page_num = left_node.next_page();
      if (next_page_num == BP_INVALID_PAGE_NUM) { // 这里已经是最后一页，说明当前扫描，没有数据
	return RC::SUCCESS;
      }

      tree_handler_.disk_buffer_pool_->unpin_page(left_frame_);
      rc = tree_handler_.disk_buffer_pool_->get_this_page(next_page_num, &left_frame_);
      if (rc != RC::SUCCESS) {
	LOG_WARN("failed to fetch next page. page num=%d, rc=%d:%s", next_page_num, rc, strrc(rc));
	return rc;
      }

      left_index = 0;
    }
    iter_index_ = left_index;
  }

  // 没有指定右边界范围，那么就返回右边界最大值
  if (nullptr == right_user_key) {
    rc = tree_handler_.right_most_page(right_frame_);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch right most page. rc=%d:%s", rc, strrc(rc));
      return rc;
    }

    LeafIndexNodeHandler node(tree_handler_.file_header_, right_frame_);
    end_index_ = node.size() - 1;
  } else {

    char *right_key = nullptr;
    char *fixed_right_key = const_cast<char *>(right_user_key);
    bool should_include_after_fix = false;
    if (tree_handler_.file_header_.attr_type == CHARS) {
      rc = fix_user_key(right_user_key, right_len, false/*want_greater*/, &fixed_right_key, &should_include_after_fix);
      if (rc != RC::SUCCESS) {
	LOG_WARN("failed to fix right user key. rc=%s", strrc(rc));
	return rc;
      }

      if (should_include_after_fix) {
	right_inclusive = true;
      }
    }
    if (right_inclusive) {
      right_key = tree_handler_.make_key(fixed_right_key, *RID::max());
    } else {
      right_key = tree_handler_.make_key(fixed_right_key, *RID::min());
    }

    if (fixed_right_key != right_user_key) {
      delete[] fixed_right_key;
      fixed_right_key = nullptr;
    }

    rc = tree_handler_.find_leaf(right_key, right_frame_);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to find left page. rc=%d:%s", rc, strrc(rc));
      tree_handler_.free_key(right_key);
      return rc;
    }

    LeafIndexNodeHandler right_node(tree_handler_.file_header_, right_frame_);
    int right_index = right_node.lookup(tree_handler_.key_comparator_, right_key);
    tree_handler_.free_key(right_key);
    // lookup 返回的是适合插入的位置，需要根据实际情况做调整
    // 通常情况下需要找到上一个位置
    if (right_index > 0) {
      right_index--;
    } else {
      // 实际上，只有最左边的叶子节点查找时，lookup 才可能返回0
      // 其它的叶子节点都不可能返回0，所以这段逻辑其实是可以简化的
      const PageNum prev_page_num = right_node.prev_page();
      if (prev_page_num == BP_INVALID_PAGE_NUM) {
	end_index_ = -1;
	return RC::SUCCESS;
      }

      tree_handler_.disk_buffer_pool_->unpin_page(right_frame_);
      rc = tree_handler_.disk_buffer_pool_->get_this_page(prev_page_num, &right_frame_);
      if (rc != RC::SUCCESS) {
	LOG_WARN("failed to fetch prev page num. page num=%d, rc=%d:%s", prev_page_num, rc, strrc(rc));
	return rc;
      }

      LeafIndexNodeHandler tmp_node(tree_handler_.file_header_, right_frame_);
      right_index = tmp_node.size() - 1;
    }
    end_index_ = right_index;
  }

  // 判断是否左边界比右边界要靠后
  // 两个边界最多会多一页
  // 查找不存在的元素，或者不存在的范围数据时，可能会存在这个问题
  if (left_frame_->page_num() == right_frame_->page_num() &&
      iter_index_ > end_index_) {
    end_index_ = -1;
  } else {
    LeafIndexNodeHandler left_node(tree_handler_.file_header_, left_frame_);
    LeafIndexNodeHandler right_node(tree_handler_.file_header_, right_frame_);
    if (left_node.prev_page() == right_node.page_num()) {
      end_index_ = -1;
    }
  }
  return RC::SUCCESS;
}

RC BplusTreeScanner::next_entry(RID *rid)
{
  if (-1 == end_index_) {
    return RC::RECORD_EOF;
  }

  LeafIndexNodeHandler node(tree_handler_.file_header_, left_frame_);
  memcpy(rid, node.value_at(iter_index_), sizeof(*rid));

  if (left_frame_->page_num() == right_frame_->page_num() &&
      iter_index_ == end_index_) {
    end_index_ = -1;
    return RC::SUCCESS;
  }

  if (iter_index_ < node.size() - 1) {
    ++iter_index_;
    return RC::SUCCESS;
  }

  RC rc = RC::SUCCESS;
  if (left_frame_->page_num() != right_frame_->page_num()) {
    PageNum page_num = node.next_page();
    tree_handler_.disk_buffer_pool_->unpin_page(left_frame_);
    if (page_num == BP_INVALID_PAGE_NUM) {
      left_frame_ = nullptr;
      LOG_WARN("got invalid next page. page num=%d", page_num);
      rc = RC::INTERNAL;
    } else {
      rc = tree_handler_.disk_buffer_pool_->get_this_page(page_num, &left_frame_);
      if (rc != RC::SUCCESS) {
	left_frame_ = nullptr;
        LOG_WARN("failed to fetch next page. page num=%d, rc=%d:%s", page_num, rc, strrc(rc));
        return rc;
      }

      iter_index_ = 0;
    }
  } else if (end_index_ != -1) {
    LOG_WARN("should have more pages but not. left page=%d, right page=%d",
	     left_frame_->page_num(), right_frame_->page_num());
    rc = RC::INTERNAL;
  }
  return rc;
}

RC BplusTreeScanner::close()
{
  if (left_frame_ != nullptr) {
    tree_handler_.disk_buffer_pool_->unpin_page(left_frame_);
    left_frame_ = nullptr;
  }
  if (right_frame_ != nullptr) {
    tree_handler_.disk_buffer_pool_->unpin_page(right_frame_);
    right_frame_ = nullptr;
  }
  end_index_ = -1;
  inited_ = false;
  LOG_INFO("bplus tree scanner closed");
  return RC::SUCCESS;
}

RC BplusTreeScanner::fix_user_key(const char *user_key, int key_len, bool want_greater,
			      char **fixed_key, bool *should_inclusive)
{
  if (nullptr == fixed_key || nullptr == should_inclusive) {
    return RC::INVALID_ARGUMENT;
  }

  // 这里很粗暴，变长字段才需要做调整，其它默认都不需要做调整
  assert(tree_handler_.file_header_.attr_type == CHARS);
  assert(strlen(user_key) >= static_cast<size_t>(key_len));
  
  *should_inclusive = false;
  
  int32_t attr_length = tree_handler_.file_header_.attr_length;
  char *key_buf = new (std::nothrow)char [attr_length];
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
  *should_inclusive = true;
  if (want_greater) {
    key_buf[attr_length - 1]++;
  }
  
  *fixed_key = key_buf;
  return RC::SUCCESS;
}
