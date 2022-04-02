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
IndexNodeHandler::IndexNodeHandler(const IndexFileHeader &header, BPPageHandle &page_handle)
  : header_(header), page_num_(page_handle.page_num()), node_((IndexNode *)page_handle.data())
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
LeafIndexNodeHandler::LeafIndexNodeHandler(const IndexFileHeader &header, BPPageHandle &page_handle)
  : IndexNodeHandler(header, page_handle), leaf_node_((LeafIndexNode *)page_handle.data())
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
  int i = 0;
  for ( ; i < size; i++) {
    int result = comparator(key, __key_at(i));
    if (0 == result) {
      if (found) {
	*found = true;
      }
      return i;
    }
    if (result < 0) {
      break;
    }
  }
  if (found) {
    *found = false;
  }
  return i;
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

RC LeafIndexNodeHandler::move_half_to(LeafIndexNodeHandler &other, DiskBufferPool *bp, int file_id)
{
  const int size = this->size();
  const int move_index = size / 2;

  memcpy(other.__item_at(0), this->__item_at(move_index), item_size() * (size - move_index));
  other.increase_size(size - move_index);
  this->increase_size(- ( size - move_index));
  return RC::SUCCESS;
}
RC LeafIndexNodeHandler::move_first_to_end(LeafIndexNodeHandler &other, DiskBufferPool *disk_buffer_pool, int file_id)
{
  other.append(__item_at(0));

  if (size() >= 1) {
    memmove(__item_at(0), __item_at(1), (size() - 1) * item_size() );
  }
  increase_size(-1);
  return RC::SUCCESS;
}

RC LeafIndexNodeHandler::move_last_to_front(LeafIndexNodeHandler &other, DiskBufferPool *bp, int file_id)
{
  other.preappend(__item_at(size() - 1));

  increase_size(-1);
  return RC::SUCCESS;
}
/**
 * move all items to left page
 */
RC LeafIndexNodeHandler::move_to(LeafIndexNodeHandler &other, DiskBufferPool *bp, int file_id)
{
  memcpy(other.__item_at(other.size()), this->__item_at(0), this->size() * item_size());
  other.increase_size(this->size());
  this->increase_size(- this->size());

  other.set_next_page(this->next_page());

  PageNum next_right_page_num = this->next_page();
  if (next_right_page_num != BP_INVALID_PAGE_NUM) {
    BPPageHandle next_right_page_handle;
    RC rc = bp->get_this_page(file_id, next_right_page_num, &next_right_page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch next right page. page number:%d. rc=%d:%s", next_right_page_num, rc, strrc(rc));
      return rc;
    }

    LeafIndexNodeHandler next_right_node(header_, next_right_page_handle);
    next_right_node.set_prev_page(other.page_num());
    next_right_page_handle.mark_dirty();
    bp->unpin_page(&next_right_page_handle);
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

bool LeafIndexNodeHandler::validate(const KeyComparator &comparator, DiskBufferPool *bp, int file_id) const
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

  BPPageHandle parent_page_handle;
  RC rc = bp->get_this_page(file_id, parent_page_num, &parent_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch parent page. page num=%d, rc=%d:%s",
	     parent_page_num, rc, strrc(rc));
    return false;
  }

  InternalIndexNodeHandler parent_node(header_, parent_page_handle);
  int index_in_parent = parent_node.value_index(this->page_num());
  if (index_in_parent < 0) {
    LOG_WARN("invalid leaf node. cannot find index in parent. this page num=%d, parent page num=%d",
	     this->page_num(), parent_page_num);
    bp->unpin_page(&parent_page_handle);
    return false;
  }

  if (0 != index_in_parent) {
    int cmp_result = comparator(__key_at(0), parent_node.key_at(index_in_parent));
    if (cmp_result < 0) {
      LOG_WARN("invalid leaf node. first item should be greate than or equal to parent item. " \
	       "this page num=%d, parent page num=%d, index in parent=%d",
	       this->page_num(), parent_node.page_num(), index_in_parent);
      bp->unpin_page(&parent_page_handle);
      return false;
    }
  }

  if (index_in_parent < parent_node.size() - 1) {
    int cmp_result = comparator(__key_at(size() - 1), parent_node.key_at(index_in_parent + 1));
    if (cmp_result >= 0) {
      LOG_WARN("invalid leaf node. last item should be less than the item at the first after item in parent." \
	       "this page num=%d, parent page num=%d, parent item to compare=%d",
	       this->page_num(), parent_node.page_num(), index_in_parent + 1);
      bp->unpin_page(&parent_page_handle);
      return false;
    }
  }
  bp->unpin_page(&parent_page_handle);
  return true;
}

/////////////////////////////////////////////////////////////////////////////////
InternalIndexNodeHandler::InternalIndexNodeHandler(const IndexFileHeader &header, BPPageHandle &page_handle)
  : IndexNodeHandler(header, page_handle), internal_node_((InternalIndexNode *)page_handle.data())
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

RC InternalIndexNodeHandler::move_half_to(InternalIndexNodeHandler &other, DiskBufferPool *bp, int file_id)
{
  const int size = this->size();
  const int move_index = size / 2;
  RC rc = other.copy_from(this->__item_at(move_index), size - move_index, bp, file_id);
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
  int i = 1;
  const int size = this->size();
  for ( ; i < size; i++) {
    int result = comparator(key, __key_at(i));
    if (result == 0) {
      if (found) {
	*found = true;
      }
      if (insert_position) {
	*insert_position = i;
      }
      return i;
    }
    if (result < 0) {
      if (found) {
	*found = false;
      }
      if (insert_position) {
	*insert_position = i;
      }

      return i - 1;
    }
  }
  if (found) {
    *found = false;
  }
  if (insert_position) {
    *insert_position = size;
  }
  return size - 1;
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

RC InternalIndexNodeHandler::move_to(InternalIndexNodeHandler &other, DiskBufferPool *disk_buffer_pool, int file_id)
{
  RC rc = other.copy_from(__item_at(0), size(), disk_buffer_pool, file_id);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to copy items to other node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  increase_size(- this->size());
  return RC::SUCCESS;
}

RC InternalIndexNodeHandler::move_first_to_end(InternalIndexNodeHandler &other, DiskBufferPool *disk_buffer_pool, int file_id)
{
  RC rc = other.append(__item_at(0), disk_buffer_pool, file_id);
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

RC InternalIndexNodeHandler::move_last_to_front(InternalIndexNodeHandler &other, DiskBufferPool *bp, int file_id)
{
  RC rc = other.preappend(__item_at(size() - 1), bp, file_id);
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
RC InternalIndexNodeHandler::copy_from(const char *items, int num, DiskBufferPool *disk_buffer_pool, int file_id)
{
  memcpy(__item_at(this->size()), items, num * item_size());

  RC rc = RC::SUCCESS;
  PageNum this_page_num = this->page_num();
  BPPageHandle page_handle;
  for (int i = 0; i < num; i++) {
    const PageNum page_num = *(const PageNum *)((items + i * item_size()) + key_size());
    rc = disk_buffer_pool->get_this_page(file_id, page_num, &page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to set child's page num. child page num:%d, this page num=%d, rc=%d:%s",
	       page_num, this_page_num, rc, strrc(rc));
      return rc;
    }
    IndexNodeHandler child_node(header_, page_handle);
    child_node.set_parent_page_num(this_page_num);
    page_handle.mark_dirty();
    disk_buffer_pool->unpin_page(&page_handle);
  }
  increase_size(num);
  return rc;
}

RC InternalIndexNodeHandler::append(const char *item, DiskBufferPool *bp, int file_id)
{
  return this->copy_from(item, 1, bp, file_id);
}

RC InternalIndexNodeHandler::preappend(const char *item, DiskBufferPool *bp, int file_id)
{
  PageNum child_page_num = *(PageNum *)(item + key_size());
  BPPageHandle page_handle;
  RC rc = bp->get_this_page(file_id, child_page_num, &page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch child page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  IndexNodeHandler child_node(header_, page_handle);
  child_node.set_parent_page_num(this->page_num());

  page_handle.mark_dirty();
  bp->unpin_page(&page_handle);

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

bool InternalIndexNodeHandler::validate(const KeyComparator &comparator, DiskBufferPool *bp, int file_id) const
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
      BPPageHandle child_page_handle;
      RC rc = bp->get_this_page(file_id, page_num, &child_page_handle);
      if (rc != RC::SUCCESS) {
	LOG_WARN("failed to fetch child page while validate internal page. page num=%d, rc=%d:%s",
		 page_num, rc, strrc(rc));
      } else {
	IndexNodeHandler child_node(header_, child_page_handle);
	if (child_node.parent_page_num() != this->page_num()) {
	  LOG_WARN("child's parent page num is invalid. child page num=%d, parent page num=%d, this page num=%d",
		   child_node.page_num(), child_node.parent_page_num(), this->page_num());
	  result = false;
	}
	bp->unpin_page(&child_page_handle);
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

  BPPageHandle parent_page_handle;
  RC rc = bp->get_this_page(file_id, parent_page_num, &parent_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch parent page. page num=%d, rc=%d:%s", parent_page_num, rc, strrc(rc));
    return false;
  }

  InternalIndexNodeHandler parent_node(header_, parent_page_handle);
  int index_in_parent = parent_node.value_index(this->page_num());
  if (index_in_parent < 0) {
    LOG_WARN("invalid internal node. cannot find index in parent. this page num=%d, parent page num=%d",
	     this->page_num(), parent_page_num);
    bp->unpin_page(&parent_page_handle);
    return false;
  }

  if (0 != index_in_parent) {
    int cmp_result = comparator(__key_at(1), parent_node.key_at(index_in_parent));
    if (cmp_result < 0) {
      LOG_WARN("invalid internal node. the second item should be greate than or equal to parent item. " \
	       "this page num=%d, parent page num=%d, index in parent=%d",
	       this->page_num(), parent_node.page_num(), index_in_parent);
      bp->unpin_page(&parent_page_handle);
      return false;
    }
  }

  if (index_in_parent < parent_node.size() - 1) {
    int cmp_result = comparator(__key_at(size() - 1), parent_node.key_at(index_in_parent + 1));
    if (cmp_result >= 0) {
      LOG_WARN("invalid internal node. last item should be less than the item at the first after item in parent." \
	       "this page num=%d, parent page num=%d, parent item to compare=%d",
	       this->page_num(), parent_node.page_num(), index_in_parent + 1);
      bp->unpin_page(&parent_page_handle);
      return false;
    }
  }
  bp->unpin_page(&parent_page_handle);

  return result;
}

/////////////////////////////////////////////////////////////////////////////////

RC BplusTreeHandler::sync()
{
  return disk_buffer_pool_->purge_all_pages(file_id_);
}

RC BplusTreeHandler::create(const char *file_name, AttrType attr_type, int attr_length,
			    int internal_max_size /* = -1*/, int leaf_max_size /* = -1 */)
{
  DiskBufferPool *disk_buffer_pool = theGlobalDiskBufferPool();
  RC rc = disk_buffer_pool->create_file(file_name);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to create file. file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    return rc;
  }
  LOG_INFO("Successfully create index file:%s", file_name);

  int file_id;
  rc = disk_buffer_pool->open_file(file_name, &file_id);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to open file. file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    return rc;
  }
  LOG_INFO("Successfully open index file %s.", file_name);

  BPPageHandle header_page_handle;
  rc = disk_buffer_pool->allocate_page(file_id, &header_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to allocate header page for bplus tree. rc=%d:%s", rc, strrc(rc));
    disk_buffer_pool->close_file(file_id);
    return rc;
  }

  if (header_page_handle.page_num() != FIRST_INDEX_PAGE) {
    LOG_WARN("header page num should be %d but got %d. is it a new file : %s",
	     FIRST_INDEX_PAGE, header_page_handle.page_num(), file_name);
    disk_buffer_pool->close_file(file_id);
    return RC::INTERNAL;
  }

  if (internal_max_size < 0) {
    internal_max_size = calc_internal_page_capacity(attr_length);
  }
  if (leaf_max_size < 0) {
    leaf_max_size = calc_leaf_page_capacity(attr_length);
  }
  char *pdata = header_page_handle.data();
  IndexFileHeader *file_header = (IndexFileHeader *)pdata;
  file_header->attr_length = attr_length;
  file_header->key_length = attr_length + sizeof(RID);
  file_header->attr_type = attr_type;
  file_header->internal_max_size = internal_max_size;
  file_header->leaf_max_size = leaf_max_size;
  file_header->root_page = BP_INVALID_PAGE_NUM;

  header_page_handle.mark_dirty();
  disk_buffer_pool->unpin_page(&header_page_handle);

  disk_buffer_pool_ = disk_buffer_pool;
  file_id_ = file_id;

  memcpy(&file_header_, pdata, sizeof(file_header_));
  header_dirty_ = false;

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
  if (file_id_ >= 0) {
    LOG_WARN("%s has been opened before index.open.", file_name);
    return RC::RECORD_OPENNED;
  }

  DiskBufferPool *disk_buffer_pool = theGlobalDiskBufferPool();
  int file_id = 0;
  RC rc = disk_buffer_pool->open_file(file_name, &file_id);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to open file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    return rc;
  }

  BPPageHandle page_handle;
  rc = disk_buffer_pool->get_this_page(file_id, FIRST_INDEX_PAGE, &page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get first page file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    disk_buffer_pool_->close_file(file_id);
    return rc;
  }

  char *pdata = page_handle.data();
  memcpy(&file_header_, pdata, sizeof(IndexFileHeader));
  header_dirty_ = false;
  disk_buffer_pool_ = disk_buffer_pool;
  file_id_ = file_id;

  mem_pool_item_ = new common::MemPoolItem(file_name);
  if (mem_pool_item_->init(file_header_.key_length) < 0) {
    LOG_WARN("Failed to init memory pool for index %s", file_name);
    close();
    return RC::NOMEM;
  }

  // close old page_handle
  disk_buffer_pool->unpin_page(&page_handle);

  key_comparator_.init(file_header_.attr_type, file_header_.attr_length);
  LOG_INFO("Successfully open index %s", file_name);
  return RC::SUCCESS;
}

RC BplusTreeHandler::close()
{
  if (file_id_ != -1) {

    disk_buffer_pool_->close_file(file_id_);
    file_id_ = -1;

    delete mem_pool_item_;
    mem_pool_item_ = nullptr;
  }

  disk_buffer_pool_ = nullptr;
  return RC::SUCCESS;
}

RC BplusTreeHandler::print_leaf(BPPageHandle &page_handle)
{
  LeafIndexNodeHandler leaf_node(file_header_, page_handle);
  LOG_INFO("leaf node: %s", to_string(leaf_node, key_printer_).c_str());
  disk_buffer_pool_->unpin_page(&page_handle);
  return RC::SUCCESS;
}

RC BplusTreeHandler::print_internal_node_recursive(BPPageHandle &page_handle)
{
  RC rc = RC::SUCCESS;
  LOG_INFO("bplus tree. file header: %s", file_header_.to_string().c_str());
  InternalIndexNodeHandler internal_node(file_header_, page_handle);
  LOG_INFO("internal node: %s", to_string(internal_node, key_printer_).c_str());

  int node_size = internal_node.size();
  for (int i = 0; i < node_size; i++) {
    PageNum page_num = internal_node.value_at(i);
    BPPageHandle child_page_handle;
    rc = disk_buffer_pool_->get_this_page(file_id_, page_num, &child_page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch child page. page id=%d, rc=%d:%s", page_num, rc, strrc(rc));
      disk_buffer_pool_->unpin_page(&page_handle);
      return rc;
    }

    IndexNodeHandler node(file_header_, child_page_handle);
    if (node.is_leaf()) {
      rc = print_leaf(child_page_handle);
    } else {
      rc = print_internal_node_recursive(child_page_handle);
    }
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to print node. page id=%d, rc=%d:%s", child_page_handle.page_num(), rc, strrc(rc));
      disk_buffer_pool_->unpin_page(&page_handle);
      return rc;
    }
  }

  disk_buffer_pool_->unpin_page(&page_handle);
  return RC::SUCCESS;
}

RC BplusTreeHandler::print_tree()
{
  if (file_id_ < 0) {
    LOG_WARN("Index hasn't been created or opened, fail to print");
    return RC::SUCCESS;
  }
  if (is_empty()) {
    LOG_INFO("tree is empty");
    return RC::SUCCESS;
  }

  BPPageHandle page_handle;
  PageNum page_num = file_header_.root_page;
  RC rc = disk_buffer_pool_->get_this_page(file_id_, page_num, &page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch page. page id=%d, rc=%d:%s", page_num, rc, strrc(rc));
    return rc;
  }
  
  IndexNodeHandler node(file_header_, page_handle);
  if (node.is_leaf()) {
    rc = print_leaf(page_handle);
  } else {
    rc = print_internal_node_recursive(page_handle);
  }
  return rc;
}

RC BplusTreeHandler::print_leafs()
{
  if (is_empty()) {
    LOG_INFO("empty tree");
    return RC::SUCCESS;
  }

  BPPageHandle page_handle;
  
  RC rc = left_most_page(page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get left most page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  while (page_handle.page_num() != BP_INVALID_PAGE_NUM) {
    LeafIndexNodeHandler leaf_node(file_header_, page_handle);
    LOG_INFO("leaf info: %s", to_string(leaf_node, key_printer_).c_str());

    PageNum next_page_num = leaf_node.next_page();
    disk_buffer_pool_->unpin_page(&page_handle);

    if (next_page_num == BP_INVALID_PAGE_NUM) {
      break;
    }

    rc = disk_buffer_pool_->get_this_page(file_id_, next_page_num, &page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get next page. page id=%d, rc=%d:%s", next_page_num, rc, strrc(rc));
      return rc;
    }
  }
  return rc;
}

bool BplusTreeHandler::validate_node_recursive(BPPageHandle &page_handle)
{
  bool result = true;
  IndexNodeHandler node(file_header_, page_handle);
  if (node.is_leaf()) {
    LeafIndexNodeHandler leaf_node(file_header_, page_handle);
    result = leaf_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
  } else {
    InternalIndexNodeHandler internal_node(file_header_, page_handle);
    result = internal_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
    for (int i = 0; result && i < internal_node.size(); i++) {
      PageNum page_num = internal_node.value_at(i);
      BPPageHandle child_page_handle;
      RC rc = disk_buffer_pool_->get_this_page(file_id_, page_num, &child_page_handle);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to fetch child page.page id=%d, rc=%d:%s", page_num, rc, strrc(rc));
        result = false;
        break;
      }

      result = validate_node_recursive(child_page_handle);
    }
  }

  disk_buffer_pool_->unpin_page(&page_handle);
  return result;
}

bool BplusTreeHandler::validate_leaf_link()
{
  if (is_empty()) {
    return true;
  }

  BPPageHandle page_handle;
  RC rc = left_most_page(page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch left most page. rc=%d:%s", rc, strrc(rc));
    return false;
  }

  PageNum prev_page_num = BP_INVALID_PAGE_NUM;

  LeafIndexNodeHandler leaf_node(file_header_, page_handle);
  if (leaf_node.prev_page() != prev_page_num) {
    LOG_WARN("invalid page. current_page_num=%d, prev page num should be %d but got %d",
  	       page_handle.page_num(), prev_page_num, leaf_node.prev_page());
    return false;
  }
  PageNum next_page_num = leaf_node.next_page();

  prev_page_num = page_handle.page_num();
  char *prev_key = (char *)mem_pool_item_->alloc();
  memcpy(prev_key, leaf_node.key_at(leaf_node.size() - 1), file_header_.key_length);
  disk_buffer_pool_->unpin_page(&page_handle);

  bool result = true;
  while (result && next_page_num != BP_INVALID_PAGE_NUM) {
    rc = disk_buffer_pool_->get_this_page(file_id_, next_page_num, &page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch next page. page num=%d, rc=%d:%s", next_page_num, rc, strrc(rc));
      return false;
    }

    LeafIndexNodeHandler leaf_node(file_header_, page_handle);
    if (leaf_node.prev_page() != prev_page_num) {
      LOG_WARN("invalid page. current_page_num=%d, prev page num should be %d but got %d",
	       page_handle.page_num(), prev_page_num, leaf_node.prev_page());
      result = false;
    }
    if (key_comparator_(prev_key, leaf_node.key_at(0)) >= 0) {
      LOG_WARN("invalid page. current first key is not bigger than last");
      result = false;
    }

    next_page_num = leaf_node.next_page();
    memcpy(prev_key, leaf_node.key_at(leaf_node.size() - 1), file_header_.key_length);
    prev_page_num = page_handle.page_num();
    disk_buffer_pool_->unpin_page(&page_handle);
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

  BPPageHandle page_handle;
  RC rc = disk_buffer_pool_->get_this_page(file_id_, file_header_.root_page, &page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch root page. page id=%d, rc=%d:%s", file_header_.root_page, rc, strrc(rc));
    return rc;
  }

  if (!validate_node_recursive(page_handle) || !validate_leaf_link()) {
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

RC BplusTreeHandler::find_leaf(const char *key, BPPageHandle &page_handle)
{
  return find_leaf_internal(
			    [&](InternalIndexNodeHandler &internal_node) {
			      return internal_node.value_at(internal_node.lookup(key_comparator_, key));
			    },
			    page_handle);
}

RC BplusTreeHandler::left_most_page(BPPageHandle &page_handle)
{
  return find_leaf_internal(
			    [&](InternalIndexNodeHandler &internal_node) {
			      return internal_node.value_at(0);
			    },
			    page_handle
			    );
}
RC BplusTreeHandler::right_most_page(BPPageHandle &page_handle)
{
  return find_leaf_internal(
			    [&](InternalIndexNodeHandler &internal_node) {
			      return internal_node.value_at(internal_node.size() - 1);
			    },
			    page_handle
			    );
}

RC BplusTreeHandler::find_leaf_internal(const std::function<PageNum(InternalIndexNodeHandler &)> &child_page_getter,
					BPPageHandle &page_handle)
{
  if (is_empty()) {
    return RC::EMPTY;
  }

  RC rc = disk_buffer_pool_->get_this_page(file_id_, file_header_.root_page, &page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch root page. page id=%d, rc=%d:%s", file_header_.root_page, rc, strrc(rc));
    return rc;
  }

  IndexNode *node = (IndexNode *)page_handle.data();
  while (false == node->is_leaf) {
    InternalIndexNodeHandler internal_node(file_header_, page_handle);
    PageNum page_num = child_page_getter(internal_node);

    disk_buffer_pool_->unpin_page(&page_handle);

    rc = disk_buffer_pool_->get_this_page(file_id_, page_num, &page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to load page file_id:%d, page_num:%d", file_id_, page_num);
      return rc;
    }
    node = (IndexNode *)page_handle.data();
  }

  return RC::SUCCESS;
}

RC BplusTreeHandler::insert_entry_into_leaf_node(BPPageHandle &page_handle, const char *key, const RID *rid)
{
  LeafIndexNodeHandler leaf_node(file_header_, page_handle);
  bool exists = false;
  int insert_position = leaf_node.lookup(key_comparator_, key, &exists);
  if (exists) {
    LOG_TRACE("entry exists");
    return RC::RECORD_DUPLICATE_KEY;
  }

  if (leaf_node.size() < leaf_node.max_size()) {
    leaf_node.insert(insert_position, key, (const char *)rid);
    page_handle.mark_dirty();
    disk_buffer_pool_->unpin_page(&page_handle);
    return RC::SUCCESS;
  }

  BPPageHandle new_page_handle;
  RC rc = split<LeafIndexNodeHandler>(page_handle, new_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to split leaf node. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  LeafIndexNodeHandler new_index_node(file_header_, new_page_handle);
  new_index_node.set_prev_page(page_handle.page_num());
  new_index_node.set_next_page(leaf_node.next_page());
  new_index_node.set_parent_page_num(leaf_node.parent_page_num());
  leaf_node.set_next_page(new_page_handle.page_num());

  PageNum next_page_num = new_index_node.next_page();
  if (next_page_num != BP_INVALID_PAGE_NUM) {
    BPPageHandle next_page_handle;
    rc = disk_buffer_pool_->get_this_page(file_id_, next_page_num, &next_page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch next page. page num=%d, rc=%d:%s", next_page_num, rc, strrc(rc));
      return rc;
    }

    LeafIndexNodeHandler next_node(file_header_, next_page_handle);
    next_node.set_prev_page(new_page_handle.page_num());
    disk_buffer_pool_->unpin_page(&next_page_handle);
  }

  if (insert_position < leaf_node.size()) {
    leaf_node.insert(insert_position, key, (const char *)rid);
  } else {
    new_index_node.insert(insert_position - leaf_node.size(), key, (const char *)rid);
  }

  return insert_entry_into_parent(page_handle, new_page_handle, new_index_node.key_at(0));
}

RC BplusTreeHandler::insert_entry_into_parent(BPPageHandle &page_handle, BPPageHandle &new_page_handle, const char *key)
{
  RC rc = RC::SUCCESS;

  IndexNodeHandler node_handler(file_header_, page_handle);
  IndexNodeHandler new_node_handler(file_header_, new_page_handle);
  PageNum parent_page_num = node_handler.parent_page_num();

  if (parent_page_num == BP_INVALID_PAGE_NUM) {

    // create new root page
    BPPageHandle root_page;
    rc = disk_buffer_pool_->allocate_page(file_id_, &root_page);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to allocate new root page. rc=%d:%s", rc, strrc(rc));
      return rc;
    }

    InternalIndexNodeHandler root_node(file_header_, root_page);
    root_node.init_empty();
    root_node.create_new_root(page_handle.page_num(), key, new_page_handle.page_num());
    node_handler.set_parent_page_num(root_page.page_num());
    new_node_handler.set_parent_page_num(root_page.page_num());

    page_handle.mark_dirty();
    new_page_handle.mark_dirty();
    disk_buffer_pool_->unpin_page(&page_handle);
    disk_buffer_pool_->unpin_page(&new_page_handle);

    file_header_.root_page = root_page.page_num();
    update_root_page_num(); // TODO
    root_page.mark_dirty();
    disk_buffer_pool_->unpin_page(&root_page);

    return RC::SUCCESS;

  } else {

    BPPageHandle parent_page_handle;
    rc = disk_buffer_pool_->get_this_page(file_id_, parent_page_num, &parent_page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to insert entry into leaf. rc=%d:%s", rc, strrc(rc));
      // should do more things to recover
      return rc;
    }

    InternalIndexNodeHandler node(file_header_, parent_page_handle);

    /// current node is not in full mode, insert the entry and return
    if (node.size() < node.max_size()) {
      node.insert(key, new_page_handle.page_num(), key_comparator_);
      new_node_handler.set_parent_page_num(parent_page_num);

      page_handle.mark_dirty();
      new_page_handle.mark_dirty();
      parent_page_handle.mark_dirty();
      disk_buffer_pool_->unpin_page(&page_handle);
      disk_buffer_pool_->unpin_page(&new_page_handle);
      disk_buffer_pool_->unpin_page(&parent_page_handle);

    } else {

      // we should split the node and insert the entry and then insert new entry to current node's parent
      BPPageHandle new_parent_page_handle;
      rc = split<InternalIndexNodeHandler>(parent_page_handle, new_parent_page_handle);
      if (rc != RC::SUCCESS) {
	LOG_WARN("failed to split internal node. rc=%d:%s", rc, strrc(rc));
	disk_buffer_pool_->unpin_page(&page_handle);
	disk_buffer_pool_->unpin_page(&new_page_handle);
	disk_buffer_pool_->unpin_page(&parent_page_handle);
      } else {
	// insert into left or right ? decide by key compare result
	InternalIndexNodeHandler new_node(file_header_, new_parent_page_handle);
	if (key_comparator_(key, new_node.key_at(0)) > 0) {
	  new_node.insert(key, new_page_handle.page_num(), key_comparator_);
          new_node_handler.set_parent_page_num(new_node.page_num());
	} else {
	  node.insert(key, new_page_handle.page_num(), key_comparator_);
          new_node_handler.set_parent_page_num(node.page_num());
	}

	disk_buffer_pool_->unpin_page(&page_handle);
	disk_buffer_pool_->unpin_page(&new_page_handle);
	
	rc = insert_entry_into_parent(parent_page_handle, new_parent_page_handle, new_node.key_at(0));
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
RC BplusTreeHandler::split(BPPageHandle &page_handle, BPPageHandle &new_page_handle)
{
  IndexNodeHandlerType old_node(file_header_, page_handle);

  char *new_parent_key = (char *)mem_pool_item_->alloc();
  if (new_parent_key == nullptr) {
    LOG_WARN("Failed to alloc memory for new key. size=%d", file_header_.key_length);
    return RC::NOMEM;
  }

  // add a new node
  RC rc = disk_buffer_pool_->allocate_page(file_id_, &new_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to split index page due to failed to allocate page, file_id:%d. rc=%d:%s", file_id_, rc, strrc(rc));
    return rc;
  }

  IndexNodeHandlerType new_node(file_header_, new_page_handle);
  new_node.init_empty();
  new_node.set_parent_page_num(old_node.parent_page_num());

  old_node.move_half_to(new_node, disk_buffer_pool_, file_id_);

  page_handle.mark_dirty();
  new_page_handle.mark_dirty();
  return RC::SUCCESS;
}

RC BplusTreeHandler::update_root_page_num()
{
  BPPageHandle header_page_handle;
  RC rc = disk_buffer_pool_->get_this_page(file_id_, FIRST_INDEX_PAGE, &header_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch header page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  IndexFileHeader *header = (IndexFileHeader *)header_page_handle.data();
  header->root_page = file_header_.root_page;
  header_page_handle.mark_dirty();
  disk_buffer_pool_->unpin_page(&header_page_handle);
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

  BPPageHandle page_handle;
  rc = disk_buffer_pool_->allocate_page(file_id_, &page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to allocate root page. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  LeafIndexNodeHandler leaf_node(file_header_, page_handle);
  leaf_node.init_empty();
  leaf_node.insert(0, key, (const char *)rid);
  file_header_.root_page = page_handle.page_num();
  page_handle.mark_dirty();
  disk_buffer_pool_->unpin_page(&page_handle);

  rc = update_root_page_num();
  // disk_buffer_pool_->check_all_pages_unpinned(file_id_);
  return rc;
}

char *BplusTreeHandler::make_key(const char *user_key, const RID &rid)
{
  char *key = (char *)mem_pool_item_->alloc();
  if (key == nullptr) {
    LOG_WARN("Failed to alloc memory for key. file_id:%d", file_id_);
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
  if (file_id_ < 0) {
    LOG_WARN("Index isn't ready!");
    return RC::RECORD_CLOSED;
  }

  if (user_key == nullptr || rid == nullptr) {
    LOG_WARN("Invalid arguments, key is empty or rid is empty");
    return RC::INVALID_ARGUMENT;
  }

  char *key = make_key(user_key, *rid);
  if (key == nullptr) {
    LOG_WARN("Failed to alloc memory for key. file_id:%d", file_id_);
    return RC::NOMEM;
  }

  if (is_empty()) {
    return create_new_tree(key, rid);
  }

  BPPageHandle page_handle;
  RC rc = find_leaf(key, page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to find leaf file_id:%d, %s. rc=%d:%s", file_id_, rid->to_string().c_str(), rc, strrc(rc));
    mem_pool_item_->free(key);
    return rc;
  }

  rc = insert_entry_into_leaf_node(page_handle, key, rid);
  if (rc != RC::SUCCESS) {
    LOG_TRACE("Failed to insert into leaf of index %d, rid:%s", file_id_, rid->to_string().c_str());
    disk_buffer_pool_->unpin_page(&page_handle);
    mem_pool_item_->free(key);
    // disk_buffer_pool_->check_all_pages_unpinned(file_id_);
    return rc;
  }

  mem_pool_item_->free(key);
  LOG_TRACE("insert entry success");
  // disk_buffer_pool_->check_all_pages_unpinned(file_id_);
  return RC::SUCCESS;
}

RC BplusTreeHandler::get_entry(const char *user_key, std::list<RID> &rids)
{
  if (file_id_ < 0) {
    LOG_WARN("Index isn't ready!");
    return RC::RECORD_CLOSED;
  }

  LOG_INFO("before get entry");
  disk_buffer_pool_->check_all_pages_unpinned(file_id_);

  BplusTreeScanner scanner(*this);
  RC rc = scanner.open(user_key, true/*left_inclusive*/, user_key, true/*right_inclusive*/);
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
  LOG_INFO("after get entry");
  disk_buffer_pool_->check_all_pages_unpinned(file_id_);
  return rc;
}

RC BplusTreeHandler::adjust_root(BPPageHandle &root_page_handle)
{
  IndexNodeHandler root_node(file_header_, root_page_handle);
  if (root_node.is_leaf() && root_node.size() > 0) {
    root_page_handle.mark_dirty();
    disk_buffer_pool_->unpin_page(&root_page_handle);
    return RC::SUCCESS;
  }

  if (root_node.is_leaf()) {
    // this is a leaf and an empty node
    file_header_.root_page = BP_INVALID_PAGE_NUM;
  } else {
    // this is an internal node and has only one child node
    InternalIndexNodeHandler internal_node(file_header_, root_page_handle);

    const PageNum child_page_num = internal_node.value_at(0);
    BPPageHandle child_page_handle;
    RC rc = disk_buffer_pool_->get_this_page(file_id_, child_page_num, &child_page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch child page. page num=%d, rc=%d:%s", child_page_num, rc, strrc(rc));
      return rc;
    }

    IndexNodeHandler child_node(file_header_, child_page_handle);
    child_node.set_parent_page_num(BP_INVALID_PAGE_NUM);
    disk_buffer_pool_->unpin_page(&child_page_handle);
    
    file_header_.root_page = child_page_num;
  }

  update_root_page_num();

  PageNum old_root_page_num = root_page_handle.page_num();
  disk_buffer_pool_->unpin_page(&root_page_handle);
  disk_buffer_pool_->dispose_page(file_id_, old_root_page_num);
  return RC::SUCCESS;
}
template <typename IndexNodeHandlerType>
RC BplusTreeHandler::coalesce_or_redistribute(BPPageHandle &page_handle)
{
  IndexNodeHandlerType index_node(file_header_, page_handle);
  if (index_node.size() >= index_node.min_size()) {
    disk_buffer_pool_->unpin_page(&page_handle);
    return RC::SUCCESS;
  }

  const PageNum parent_page_num = index_node.parent_page_num();
  if (BP_INVALID_PAGE_NUM == parent_page_num) {
    // this is the root page
    if (index_node.size() > 1) {
      disk_buffer_pool_->unpin_page(&page_handle);
    } else {
      // adjust the root node
      adjust_root(page_handle);
    }
    return RC::SUCCESS;
  }

  BPPageHandle parent_page_handle;
  RC rc = disk_buffer_pool_->get_this_page(file_id_, parent_page_num, &parent_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch parent page. page id=%d, rc=%d:%s", parent_page_num, rc, strrc(rc));
    disk_buffer_pool_->unpin_page(&page_handle);
    return rc;
  }

  InternalIndexNodeHandler parent_index_node(file_header_, parent_page_handle);
  int index = parent_index_node.lookup(key_comparator_, index_node.key_at(index_node.size() - 1));
  if (parent_index_node.value_at(index) != page_handle.page_num()) {
    LOG_ERROR("lookup return an invalid value. index=%d, this page num=%d, but got %d",
	      index, page_handle.page_num(), parent_index_node.value_at(index));
  }
  PageNum neighbor_page_num;
  if (index == 0) {
    neighbor_page_num = parent_index_node.value_at(1);
  } else {
    neighbor_page_num = parent_index_node.value_at(index - 1);
  }

  BPPageHandle neighbor_page_handle;
  rc = disk_buffer_pool_->get_this_page(file_id_, neighbor_page_num, &neighbor_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch neighbor page. page id=%d, rc=%d:%s", neighbor_page_num, rc, strrc(rc));
    // TODO do more thing to release resource
    disk_buffer_pool_->unpin_page(&page_handle);
    disk_buffer_pool_->unpin_page(&parent_page_handle);
    return rc;
  }

  IndexNodeHandlerType neighbor_node(file_header_, neighbor_page_handle);
  if (index_node.size() + neighbor_node.size() > index_node.max_size()) {
    rc = redistribute<IndexNodeHandlerType>(neighbor_page_handle, page_handle, parent_page_handle, index);
  } else {
    rc = coalesce<IndexNodeHandlerType>(neighbor_page_handle, page_handle, parent_page_handle, index);
  }
  return rc;
}

template <typename IndexNodeHandlerType>
RC BplusTreeHandler::coalesce(BPPageHandle &neighbor_page_handle, BPPageHandle &page_handle,
			      BPPageHandle &parent_page_handle, int index)
{
  IndexNodeHandlerType neighbor_node(file_header_, neighbor_page_handle);
  IndexNodeHandlerType node(file_header_, page_handle);

  InternalIndexNodeHandler parent_node(file_header_, parent_page_handle);

  BPPageHandle *left_page_handle = nullptr;
  BPPageHandle *right_page_handle = nullptr;
  if (index == 0) {
    // neighbor node is at right
    left_page_handle = &page_handle;
    right_page_handle = &neighbor_page_handle;
    index++;
  } else {
    left_page_handle = &neighbor_page_handle;
    right_page_handle = &page_handle;
    // neighbor is at left
  }

  IndexNodeHandlerType left_node(file_header_, *left_page_handle);
  IndexNodeHandlerType right_node(file_header_, *right_page_handle);

  parent_node.remove(index);
  // parent_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
  RC rc = right_node.move_to(left_node, disk_buffer_pool_, file_id_);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to move right node to left. rc=%d:%s", rc, strrc(rc));
    return rc;
  }
  // left_node.validate(key_comparator_);

  if (left_node.is_leaf()) {
    LeafIndexNodeHandler left_leaf_node(file_header_, *left_page_handle);
    LeafIndexNodeHandler right_leaf_node(file_header_, *right_page_handle);
    left_leaf_node.set_next_page(right_leaf_node.next_page());

    PageNum next_right_page_num = right_leaf_node.next_page();
    if (next_right_page_num != BP_INVALID_PAGE_NUM) {
      BPPageHandle next_right_page_handle;
      rc = disk_buffer_pool_->get_this_page(file_id_, next_right_page_num, &next_right_page_handle);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to fetch next right page. page number:%d. rc=%d:%s", next_right_page_num, rc, strrc(rc));
	disk_buffer_pool_->unpin_page(&page_handle);
	disk_buffer_pool_->unpin_page(&neighbor_page_handle);
	disk_buffer_pool_->unpin_page(&parent_page_handle);
        return rc;
      }

      LeafIndexNodeHandler next_right_node(file_header_, next_right_page_handle);
      next_right_node.set_prev_page(left_node.page_num());
      disk_buffer_pool_->unpin_page(&next_right_page_handle);
    }
    
  }

  PageNum right_page_num = right_page_handle->page_num();
  disk_buffer_pool_->unpin_page(left_page_handle);
  disk_buffer_pool_->unpin_page(right_page_handle);
  disk_buffer_pool_->dispose_page(file_id_, right_page_num);
  return coalesce_or_redistribute<InternalIndexNodeHandler>(parent_page_handle);
}

template <typename IndexNodeHandlerType>
RC BplusTreeHandler::redistribute(BPPageHandle &neighbor_page_handle, BPPageHandle &page_handle,
				  BPPageHandle &parent_page_handle, int index)
{
  InternalIndexNodeHandler parent_node(file_header_, parent_page_handle);
  IndexNodeHandlerType neighbor_node(file_header_, neighbor_page_handle);
  IndexNodeHandlerType node(file_header_, page_handle);
  if (neighbor_node.size() < node.size()) {
    LOG_ERROR("got invalid nodes. neighbor node size %d, this node size %d",
	      neighbor_node.size(), node.size());
  }
  if (index == 0) {
    // the neighbor is at right
    neighbor_node.move_first_to_end(node, disk_buffer_pool_, file_id_);
    // neighbor_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
    // node.validate(key_comparator_, disk_buffer_pool_, file_id_);
    parent_node.set_key_at(index + 1, neighbor_node.key_at(0));
    // parent_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
  } else {
    // the neighbor is at left
    neighbor_node.move_last_to_front(node, disk_buffer_pool_, file_id_);
    // neighbor_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
    // node.validate(key_comparator_, disk_buffer_pool_, file_id_);
    parent_node.set_key_at(index, node.key_at(0));
    // parent_node.validate(key_comparator_, disk_buffer_pool_, file_id_);
  }

  neighbor_page_handle.mark_dirty();
  page_handle.mark_dirty();
  parent_page_handle.mark_dirty();
  disk_buffer_pool_->unpin_page(&parent_page_handle);
  disk_buffer_pool_->unpin_page(&neighbor_page_handle);
  disk_buffer_pool_->unpin_page(&page_handle);
  return RC::SUCCESS;
}

RC BplusTreeHandler::delete_entry_internal(BPPageHandle &leaf_page_handle, const char *key)
{
  LeafIndexNodeHandler leaf_index_node(file_header_, leaf_page_handle);

  const int remove_count = leaf_index_node.remove(key, key_comparator_);
  if (remove_count == 0) {
    LOG_TRACE("no data to remove");
    disk_buffer_pool_->unpin_page(&leaf_page_handle);
    return RC::RECORD_RECORD_NOT_EXIST;
  }
  // leaf_index_node.validate(key_comparator_, disk_buffer_pool_, file_id_);

  leaf_page_handle.mark_dirty();

  if (leaf_index_node.size() >= leaf_index_node.min_size()) {
    disk_buffer_pool_->unpin_page(&leaf_page_handle);
    return RC::SUCCESS;
  }

  return coalesce_or_redistribute<LeafIndexNodeHandler>(leaf_page_handle);
}

RC BplusTreeHandler::delete_entry(const char *user_key, const RID *rid)
{
  if (file_id_ < 0) {
    LOG_WARN("Failed to delete index entry, due to index is't ready");
    return RC::RECORD_CLOSED;
  }

  char *key = (char *)mem_pool_item_->alloc();
  if (nullptr == key) {
    LOG_WARN("Failed to alloc memory for key. size=%d", file_header_.key_length);
    return RC::NOMEM;
  }
  memcpy(key, user_key, file_header_.attr_length);
  memcpy(key + file_header_.attr_length, rid, sizeof(*rid));

  LOG_INFO("before delete");
  disk_buffer_pool_->check_all_pages_unpinned(file_id_);
  BPPageHandle leaf_page_handle;
  RC rc = find_leaf(key, leaf_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to find leaf page. rc =%d:%s", rc, strrc(rc));
    mem_pool_item_->free(key);
    return rc;
  }
  rc = delete_entry_internal(leaf_page_handle, key);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to delete index %d", file_id_);
    mem_pool_item_->free(key);
    return rc;
  }
  mem_pool_item_->free(key);
  LOG_INFO("after delete");
  disk_buffer_pool_->check_all_pages_unpinned(file_id_);
  return RC::SUCCESS;
}

BplusTreeScanner::BplusTreeScanner(BplusTreeHandler &tree_handler) : tree_handler_(tree_handler)
{}

BplusTreeScanner::~BplusTreeScanner()
{
  close();
}

RC BplusTreeScanner::open(const char *left_user_key, bool left_inclusive,
                          const char *right_user_key, bool right_inclusive)
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
    rc = tree_handler_.left_most_page(left_page_handle_);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to find left most page. rc=%d:%s", rc, strrc(rc));
      return rc;
    }

    iter_index_ = 0;
  } else {
    char *left_key = nullptr;
    if (left_inclusive) { 
      left_key = tree_handler_.make_key(left_user_key, *RID::min());
    } else {
      left_key = tree_handler_.make_key(left_user_key, *RID::max());
    }
    rc = tree_handler_.find_leaf(left_key, left_page_handle_);

    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to find left page. rc=%d:%s", rc, strrc(rc));
      tree_handler_.free_key(left_key);
      return rc;
    }
    LeafIndexNodeHandler left_node(tree_handler_.file_header_, left_page_handle_);
    int left_index = left_node.lookup(tree_handler_.key_comparator_, left_key);
    tree_handler_.free_key(left_key);
    // lookup 返回的是适合插入的位置，还需要判断一下是否在合适的边界范围内
    if (left_index >= left_node.size()) { // 超出了当前页，就需要向后移动一个位置
      const PageNum next_page_num = left_node.next_page();
      if (next_page_num == BP_INVALID_PAGE_NUM) { // 这里已经是最后一页，说明当前扫描，没有数据
	return RC::SUCCESS;
      }

      tree_handler_.disk_buffer_pool_->unpin_page(&left_page_handle_);
      rc = tree_handler_.disk_buffer_pool_->get_this_page(tree_handler_.file_id_, next_page_num, &left_page_handle_);
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
    rc = tree_handler_.right_most_page(right_page_handle_);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch right most page. rc=%d:%s", rc, strrc(rc));
      return rc;
    }

    LeafIndexNodeHandler node(tree_handler_.file_header_, right_page_handle_);
    end_index_ = node.size() - 1;
  } else {

    char *right_key = nullptr;
    if (right_inclusive) {
      right_key = tree_handler_.make_key(right_user_key, *RID::max());
    } else {
      right_key = tree_handler_.make_key(right_user_key, *RID::min());
    }

    rc = tree_handler_.find_leaf(right_key, right_page_handle_);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to find left page. rc=%d:%s", rc, strrc(rc));
      tree_handler_.free_key(right_key);
      return rc;
    }

    LeafIndexNodeHandler right_node(tree_handler_.file_header_, right_page_handle_);
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

      tree_handler_.disk_buffer_pool_->unpin_page(&right_page_handle_);
      rc = tree_handler_.disk_buffer_pool_->get_this_page(tree_handler_.file_id_, prev_page_num, &right_page_handle_);
      if (rc != RC::SUCCESS) {
	LOG_WARN("failed to fetch prev page num. page num=%d, rc=%d:%s", prev_page_num, rc, strrc(rc));
	return rc;
      }

      LeafIndexNodeHandler tmp_node(tree_handler_.file_header_, right_page_handle_);
      right_index = tmp_node.size() - 1;
    }
    end_index_ = right_index;
  }

  // 判断是否左边界比右边界要靠后
  // 两个边界最多会多一页
  // 查找不存在的元素，或者不存在的范围数据时，可能会存在这个问题
  if (left_page_handle_.page_num() == right_page_handle_.page_num() &&
      iter_index_ > end_index_) {
    end_index_ = -1;
  } else {
    LeafIndexNodeHandler left_node(tree_handler_.file_header_, left_page_handle_);
    LeafIndexNodeHandler right_node(tree_handler_.file_header_, right_page_handle_);
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

  LeafIndexNodeHandler node(tree_handler_.file_header_, left_page_handle_);
  memcpy(rid, node.value_at(iter_index_), sizeof(*rid));

  if (left_page_handle_.page_num() == right_page_handle_.page_num() &&
      iter_index_ == end_index_) {
    end_index_ = -1;
    return RC::SUCCESS;
  }

  if (iter_index_ < node.size() - 1) {
    ++iter_index_;
    return RC::SUCCESS;
  }

  RC rc = RC::SUCCESS;
  if (left_page_handle_.page_num() != right_page_handle_.page_num()) {
    PageNum page_num = node.next_page();
    tree_handler_.disk_buffer_pool_->unpin_page(&left_page_handle_);
    if (page_num == BP_INVALID_PAGE_NUM) {
      LOG_WARN("got invalid next page. page num=%d", page_num);
      rc = RC::INTERNAL;
    } else {
      rc = tree_handler_.disk_buffer_pool_->get_this_page(tree_handler_.file_id_, page_num, &left_page_handle_);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to fetch next page. page num=%d, rc=%d:%s", page_num, rc, strrc(rc));
        return rc;
      }

      iter_index_ = 0;
    }
  } else if (end_index_ != -1) {
    LOG_WARN("should have more pages but not. left page=%d, right page=%d",
	     left_page_handle_.page_num(), right_page_handle_.page_num());
    rc = RC::INTERNAL;
  }
  return rc;
}

RC BplusTreeScanner::close()
{
  if (left_page_handle_.open) {
    tree_handler_.disk_buffer_pool_->unpin_page(&left_page_handle_);
  }
  if (right_page_handle_.open) {
    tree_handler_.disk_buffer_pool_->unpin_page(&right_page_handle_);
  }
  end_index_ = -1;
  inited_ = false;
  LOG_INFO("bplus tree scanner closed");
  return RC::SUCCESS;
}
