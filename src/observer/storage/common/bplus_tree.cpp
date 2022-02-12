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
#include "storage/common/bplus_tree.h"
#include "storage/default/disk_buffer_pool.h"
#include "rc.h"
#include "common/log/log.h"
#include "sql/parser/parse_defs.h"

#define FIRST_INDEX_PAGE 1

int float_compare(float f1, float f2)
{
  float result = f1 - f2;
  if (-1e-6 < result && result < 1e-6) {
    return 0;
  }
  return result > 0 ? 1 : -1;
}

int attribute_comp(const char *first, const char *second, AttrType attr_type, int attr_length)
{  // 简化
  int i1, i2;
  float f1, f2;
  const char *s1, *s2;
  switch (attr_type) {
    case INTS: {
      i1 = *(int *)first;
      i2 = *(int *)second;
      return i1 - i2;
    } break;
    case FLOATS: {
      f1 = *(float *)first;
      f2 = *(float *)second;
      return float_compare(f1, f2);
    } break;
    case CHARS: {
      s1 = first;
      s2 = second;
      return strncmp(s1, s2, attr_length);
    } break;
    default: {
      LOG_PANIC("Unknown attr type: %d", attr_type);
    }
  }
  return -2;  // This means error happens
}
int key_compare(AttrType attr_type, int attr_length, const char *first, const char *second)
{
  int result = attribute_comp(first, second, attr_type, attr_length);
  if (0 != result) {
    return result;
  }
  RID *rid1 = (RID *)(first + attr_length);
  RID *rid2 = (RID *)(second + attr_length);
  return RID::compare(rid1, rid2);
}

int get_page_index_capacity(int attr_length)
{

  int capacity =
      ((int)BP_PAGE_DATA_SIZE - sizeof(IndexFileHeader) - sizeof(IndexNode)) / (attr_length + 2 * sizeof(RID));
  // Here is some tricks
  // 1. reserver one pair of kV for insert operation
  // 2. make sure capacity % 2 == 0, otherwise it is likeyly to occur problem when split node
  capacity = ((capacity - RECORD_RESERVER_PAIR_NUM) / 2) * 2;
  return capacity;
}

IndexNode *BplusTreeHandler::get_index_node(char *page_data) const
{
  IndexNode *node = (IndexNode *)(page_data + sizeof(IndexFileHeader));
  node->keys = (char *)node + sizeof(IndexNode);
  node->rids = (RID *)(node->keys + (file_header_.order + RECORD_RESERVER_PAIR_NUM) * file_header_.key_length);
  return node;
}

RC BplusTreeHandler::sync()
{
  return disk_buffer_pool_->purge_all_pages(file_id_);
}

RC BplusTreeHandler::create(const char *file_name, AttrType attr_type, int attr_length)
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

  rc = disk_buffer_pool->allocate_page(file_id, &root_page_handle_);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to allocate page. file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    disk_buffer_pool->close_file(file_id);
    return rc;
  }

  char *pdata;
  rc = disk_buffer_pool->get_data(&root_page_handle_, &pdata);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get data. file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    disk_buffer_pool->close_file(file_id);
    return rc;
  }

  PageNum page_num;
  rc = disk_buffer_pool->get_page_num(&root_page_handle_, &page_num);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get page num. file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    disk_buffer_pool->close_file(file_id);
    return rc;
  }

  IndexFileHeader *file_header = (IndexFileHeader *)pdata;
  file_header->attr_length = attr_length;
  file_header->key_length = attr_length + sizeof(RID);
  file_header->attr_type = attr_type;
  file_header->order = get_page_index_capacity(attr_length);
  file_header->root_page = page_num;

  root_node_ = get_index_node(pdata);
  root_node_->init_empty(*file_header);

  disk_buffer_pool->mark_dirty(&root_page_handle_);

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

  LOG_INFO("Successfully create index %s", file_name);
  return RC::SUCCESS;
}

RC BplusTreeHandler::open(const char *file_name)
{
  if (file_id_ > 0) {
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

  char *pdata;
  rc = disk_buffer_pool->get_data(&page_handle, &pdata);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get first page data. file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    disk_buffer_pool_->close_file(file_id);
    return rc;
  }

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

  if (file_header_.root_page == FIRST_INDEX_PAGE) {
    root_node_ = get_index_node(pdata);
    root_page_handle_ = page_handle;

    LOG_INFO("Successfully open index %s", file_name);
    return RC::SUCCESS;
  }

  // close old page_handle
  disk_buffer_pool->unpin_page(&page_handle);

  LOG_INFO("Begin to load root page of index:%s, root_page:%d.", file_name, file_header_.root_page);
  rc = disk_buffer_pool->get_this_page(file_id, file_header_.root_page, &root_page_handle_);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get first page file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    disk_buffer_pool_->close_file(file_id);
    return rc;
  }

  rc = disk_buffer_pool->get_data(&root_page_handle_, &pdata);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get first page data. file name=%s, rc=%d:%s", file_name, rc, strrc(rc));
    disk_buffer_pool_->close_file(file_id);
    return rc;
  }
  root_node_ = get_index_node(pdata);

  LOG_INFO("Successfully open index %s", file_name);
  return RC::SUCCESS;
}

RC BplusTreeHandler::close()
{
  if (file_id_ != -1) {
    disk_buffer_pool_->unpin_page(&root_page_handle_);
    root_node_ = nullptr;

    disk_buffer_pool_->close_file(file_id_);
    file_id_ = -1;

    delete mem_pool_item_;
    mem_pool_item_ = nullptr;
  }

  disk_buffer_pool_ = nullptr;
  return RC::SUCCESS;
}

RC BplusTreeHandler::print_node(IndexNode *node, PageNum page_num)
{
  LOG_INFO("PageNum:%d, node {%s}\n", page_num, node->to_string(file_header_).c_str());

  if (node->is_leaf == false) {
    for (int i = 0; i <= node->key_num; i++) {
      PageNum child_page_num = node->rids[i].page_num;
      BPPageHandle page_handle;
      RC rc = disk_buffer_pool_->get_this_page(file_id_, child_page_num, &page_handle);
      if (rc != RC::SUCCESS) {
        LOG_WARN("Failed to load page file_id:%d, page_num:%d", file_id_, child_page_num);
        continue;
      }

      char *pdata;
      disk_buffer_pool_->get_data(&page_handle, &pdata);
      IndexNode *child = get_index_node(pdata);
      print_node(child, child_page_num);
      disk_buffer_pool_->unpin_page(&page_handle);
    }
  }

  return RC::SUCCESS;
}

RC BplusTreeHandler::print_tree()
{
  if (file_id_ < 0) {
    LOG_WARN("Index hasn't been created or opened, fail to print");
    return RC::SUCCESS;
  }

  int page_count;
  RC rc = disk_buffer_pool_->get_page_count(file_id_, &page_count);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get page count of index %d", file_id_);
    return rc;
  }

  LOG_INFO("\n\n\n !!!! Begin to print index %s:%d, page_count:%d, file_header:%s\n\n\n",
      mem_pool_item_->get_name().c_str(),
      file_id_,
      page_count,
      file_header_.to_string().c_str());

  print_node(root_node_, file_header_.root_page);
  return RC::SUCCESS;
}

RC BplusTreeHandler::print_leafs()
{
  PageNum page_num;
  get_first_leaf_page(&page_num);

  IndexNode *node;
  BPPageHandle page_handle;
  RC rc;
  while (page_num != -1) {
    rc = disk_buffer_pool_->get_this_page(file_id_, page_num, &page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to print leafs, due to failed to load. ");
      return rc;
    }
    char *pdata;
    disk_buffer_pool_->get_data(&page_handle, &pdata);
    node = get_index_node(pdata);
    LOG_INFO("Page:%d, Node:%s", page_num, node->to_string(file_header_).c_str());
    page_num = node->next_brother;
    disk_buffer_pool_->unpin_page(&page_handle);
  }

  return RC::SUCCESS;
}

bool BplusTreeHandler::validate_node(IndexNode *node)
{
  if (node->key_num > file_header_.order) {
    LOG_WARN("NODE %s 's key number is invalid", node->to_string(file_header_).c_str());
    return false;
  }
  if (node->parent != -1) {
    if (node->key_num < file_header_.order / 2) {
      LOG_WARN("NODE %s 's key number is invalid", node->to_string(file_header_).c_str());
      return false;
    }
  } else {
    // node is root
    if (node->is_leaf == false) {

      if (node->key_num < 1) {
        LOG_WARN("NODE %s 's key number is invalid", node->to_string(file_header_).c_str());
        return false;
      }
    }
  }

  if (node->is_leaf && node->prev_brother != -1) {
    char *first_key = node->keys;
    bool found = false;

    PageNum parent_page = node->parent;
    while (parent_page != -1) {
      BPPageHandle parent_handle;
      RC rc = disk_buffer_pool_->get_this_page(file_id_, parent_page, &parent_handle);
      if (rc != RC::SUCCESS) {
        LOG_WARN("Failed to check parent's keys, file_id:%d", file_id_);

        return false;
      }

      char *pdata;
      disk_buffer_pool_->get_data(&parent_handle, &pdata);
      IndexNode *parent = get_index_node(pdata);
      for (int i = 0; i < parent->key_num; i++) {
        char *cur_key = parent->keys + i * file_header_.key_length;
        int tmp = key_compare(file_header_.attr_type, file_header_.attr_length, first_key, cur_key);
        if (tmp == 0) {
          found = true;

          break;
        } else if (tmp < 0) {
          break;
        }
      }
      disk_buffer_pool_->unpin_page(&parent_handle);
      if (found == true) {
        break;
      }

      parent_page = parent->parent;
    }

    if (found == false) {
      LOG_WARN("Failed to find leaf's first key in internal node. leaf:%s, file_id:%d",
          node->to_string(file_header_).c_str(),
          file_id_);
      return false;
    }
  }

  bool ret = false;
  char *last_key = node->keys;
  char *cur_key;
  for (int i = 0; i < node->key_num; i++) {
    int tmp;
    cur_key = node->keys + i * file_header_.key_length;
    if (i > 0) {
      tmp = key_compare(file_header_.attr_type, file_header_.attr_length, cur_key, last_key);
      if (tmp < 0) {
        LOG_WARN("NODE %s 's key sequence is wrong", node->to_string(file_header_).c_str());
        return false;
      }
    }
    last_key = cur_key;
    if (node->is_leaf) {
      continue;
    }

    PageNum child_page = node->rids[i].page_num;
    BPPageHandle child_handle;
    RC rc = disk_buffer_pool_->get_this_page(file_id_, child_page, &child_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN(
          "Failed to validte node's child %d, file_id:%d, node:%s", i, file_id_, node->to_string(file_header_).c_str());
      continue;
    }
    char *pdata;
    disk_buffer_pool_->get_data(&child_handle, &pdata);
    IndexNode *child = get_index_node(pdata);

    char *child_last_key = child->keys + (child->key_num - 1) * file_header_.key_length;
    tmp = key_compare(file_header_.attr_type, file_header_.attr_length, cur_key, child_last_key);
    if (tmp <= 0) {
      LOG_WARN("Child's last key is bigger than current key, child:%s, current:%s, file_id:%d",
          child->to_string(file_header_).c_str(),
          node->to_string(file_header_).c_str(),
          file_id_);
      disk_buffer_pool_->unpin_page(&child_handle);
      return false;
    }

    ret = validate_node(child);
    if (ret == false) {
      disk_buffer_pool_->unpin_page(&child_handle);
      return false;
    }

    BPPageHandle next_child_handle;
    PageNum next_child_page = node->rids[i + 1].page_num;
    rc = disk_buffer_pool_->get_this_page(file_id_, next_child_page, &next_child_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN(
          "Failed to validte node's child %d, file_id:%d, node:%s", i, file_id_, node->to_string(file_header_).c_str());
      disk_buffer_pool_->unpin_page(&child_handle);
      continue;
    }
    disk_buffer_pool_->get_data(&next_child_handle, &pdata);
    IndexNode *next_child = get_index_node(pdata);

    char *first_next_child_key = next_child->keys;
    tmp = key_compare(file_header_.attr_type, file_header_.attr_length, cur_key, first_next_child_key);
    if (next_child->is_leaf) {
      if (tmp != 0) {
        LOG_WARN("Next child's first key isn't equal current key, next_child:%s, current:%s, file_id:%d",
            next_child->to_string(file_header_).c_str(),
            node->to_string(file_header_).c_str(),
            file_id_);
        disk_buffer_pool_->unpin_page(&next_child_handle);
        disk_buffer_pool_->unpin_page(&child_handle);
        return false;
      }
    } else {
      if (tmp >= 0) {
        LOG_WARN("Next child's first key isn't equal current key, next_child:%s, current:%s, file_id:%d",
            next_child->to_string(file_header_).c_str(),
            node->to_string(file_header_).c_str(),
            file_id_);
        disk_buffer_pool_->unpin_page(&next_child_handle);
        disk_buffer_pool_->unpin_page(&child_handle);
        return false;
      }
    }

    if (i == node->key_num - 1) {
      ret = validate_node(next_child);
      if (ret == false) {
        LOG_WARN("Next child is invalid, next_child:%s, current:%s, file_id:%d",
            next_child->to_string(file_header_).c_str(),
            node->to_string(file_header_).c_str(),
            file_id_);
        disk_buffer_pool_->unpin_page(&next_child_handle);
        disk_buffer_pool_->unpin_page(&child_handle);
        return false;
      }
    }
    if (child->is_leaf) {
      if (child->next_brother != next_child_page || next_child->prev_brother != child_page) {
        LOG_WARN("The child 's next brother or the next child's previous brother isn't correct, child:%s, "
                 "next_child:%s, file_id:%d",
            child->to_string(file_header_).c_str(),
            next_child->to_string(file_header_).c_str(),
            file_id_);
        disk_buffer_pool_->unpin_page(&next_child_handle);
        disk_buffer_pool_->unpin_page(&child_handle);
        return false;
      }
    }
    disk_buffer_pool_->unpin_page(&next_child_handle);
    disk_buffer_pool_->unpin_page(&child_handle);
  }

  return true;
}

bool BplusTreeHandler::validate_leaf_link()
{
  BPPageHandle first_leaf_handle;
  IndexNode *first_leaf = root_node_;
  PageNum first_page;
  RC rc;

  while (first_leaf->is_leaf == false) {
    if (first_leaf_handle.open) {
      disk_buffer_pool_->unpin_page(&first_leaf_handle);
    }
    first_page = first_leaf->rids[0].page_num;
    rc = disk_buffer_pool_->get_this_page(file_id_, first_page, &first_leaf_handle);
    if (rc != RC::SUCCESS) {
      return false;
    }

    char *pdata;
    disk_buffer_pool_->get_data(&first_leaf_handle, &pdata);
    first_leaf = get_index_node(pdata);
  }

  if (first_leaf_handle.open == false) {
    // only root node
    if (first_leaf->prev_brother != -1 || first_leaf->next_brother != -1) {
      LOG_WARN("root node is the only node, but either root node's previous brother or next brother is wrong, root:%s, "
               "file_id:%s",
          first_leaf->to_string(file_header_).c_str(),
          file_id_);
      return false;
    }
    return true;
  }

  if (first_leaf->prev_brother != -1 || first_leaf->next_brother == -1) {
    LOG_WARN("First leaf is invalid, node:%s, file_id:%d", first_leaf->to_string(file_header_).c_str(), file_id_);
    disk_buffer_pool_->unpin_page(&first_leaf_handle);
    return false;
  }

  BPPageHandle last_leaf_handle;
  IndexNode *last_leaf = root_node_;
  PageNum last_page = -1;

  while (last_leaf->is_leaf == false) {
    if (last_leaf_handle.open) {
      disk_buffer_pool_->unpin_page(&last_leaf_handle);
    }
    last_page = last_leaf->rids[last_leaf->key_num].page_num;
    rc = disk_buffer_pool_->get_this_page(file_id_, last_page, &last_leaf_handle);
    if (rc != RC::SUCCESS) {
      disk_buffer_pool_->unpin_page(&first_leaf_handle);
      return false;
    }

    char *pdata;
    disk_buffer_pool_->get_data(&last_leaf_handle, &pdata);
    last_leaf = get_index_node(pdata);
  }

  if (last_page == -1) {
    LOG_WARN("The last leaf is invalid, last leaf is root:%s, file_id:%d",
        last_leaf->to_string(file_header_).c_str(),
        file_id_);
    disk_buffer_pool_->unpin_page(&first_leaf_handle);
    return false;
  }

  if (last_leaf->next_brother != -1 || last_leaf->prev_brother == -1) {
    LOG_WARN(
        "The last leaf is invalid, last leaf:%s, file_id:%d", last_leaf->to_string(file_header_).c_str(), file_id_);
    disk_buffer_pool_->unpin_page(&first_leaf_handle);
    disk_buffer_pool_->unpin_page(&last_leaf_handle);
    return false;
  }

  std::set<PageNum> leaf_pages;
  leaf_pages.insert(first_page);

  BPPageHandle current_handle;
  IndexNode *cur_node = first_leaf;
  PageNum cur_page = first_page;

  BPPageHandle next_handle;
  IndexNode *next_node = nullptr;
  PageNum next_page = cur_node->next_brother;

  bool found = false;
  bool ret = false;

  while (next_page != -1) {
    rc = disk_buffer_pool_->get_this_page(file_id_, next_page, &next_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to check leaf link ");
      goto cleanup;
    }

    char *pdata;
    disk_buffer_pool_->get_data(&next_handle, &pdata);
    next_node = get_index_node(pdata);

    if (cur_node->next_brother != next_page || next_node->prev_brother != cur_page) {
      LOG_WARN("The leaf 's next brother or the next leaf's previous brother isn't correct, child:%s, next_child:%s, "
               "file_id:%d",
          cur_node->to_string(file_header_).c_str(),
          next_node->to_string(file_header_).c_str(),
          file_id_);
      disk_buffer_pool_->unpin_page(&next_handle);
      goto cleanup;
    }

    if (next_page == last_page) {
      found = true;
      disk_buffer_pool_->unpin_page(&next_handle);
      break;
    }

    if (leaf_pages.find(next_page) != leaf_pages.end()) {
      LOG_WARN(
          "Leaf links occur loop, current node:%s, file_id:%d", cur_node->to_string(file_header_).c_str(), file_id_);
      disk_buffer_pool_->unpin_page(&next_handle);
      goto cleanup;
    } else {
      leaf_pages.insert(next_page);
    }

    if (current_handle.open) {
      disk_buffer_pool_->unpin_page(&current_handle);
    }
    current_handle = next_handle;
    cur_node = next_node;
    cur_page = next_page;
    next_page = cur_node->next_brother;
  }

  if (found == true) {
    ret = true;
  }

cleanup:
  if (first_leaf_handle.open) {
    disk_buffer_pool_->unpin_page(&first_leaf_handle);
  }

  if (last_leaf_handle.open) {
    disk_buffer_pool_->unpin_page(&last_leaf_handle);
  }

  if (current_handle.open) {
    disk_buffer_pool_->unpin_page(&current_handle);
  }

  return ret;
}

bool BplusTreeHandler::validate_tree()
{
  IndexNode *node = root_node_;
  if (validate_node(node) == false || validate_leaf_link() == false) {
    LOG_WARN("Current B+ Tree is invalid");
    print_tree();
    return false;
  }
  return true;
}

RC BplusTreeHandler::find_leaf(const char *pkey, PageNum *leaf_page)
{
  BPPageHandle page_handle;
  IndexNode *node = root_node_;
  while (false == node->is_leaf) {
    char *pdata;
    int i;
    for (i = 0; i < node->key_num; i++) {
      int tmp =
          key_compare(file_header_.attr_type, file_header_.attr_length, pkey, node->keys + i * file_header_.key_length);
      if (tmp < 0)
        break;
    }

    if (page_handle.open == true) {
      disk_buffer_pool_->unpin_page(&page_handle);
    }

    RC rc = disk_buffer_pool_->get_this_page(file_id_, node->rids[i].page_num, &page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to load page file_id:%d, page_num:%d", file_id_, node->rids[i].page_num);
      return rc;
    }
    disk_buffer_pool_->get_data(&page_handle, &pdata);

    node = get_index_node(pdata);
  }

  if (page_handle.open == false) {
    *leaf_page = file_header_.root_page;
    return RC::SUCCESS;
  }

  disk_buffer_pool_->get_page_num(&page_handle, leaf_page);
  disk_buffer_pool_->unpin_page(&page_handle);

  return RC::SUCCESS;
}

RC BplusTreeHandler::insert_entry_into_node(IndexNode *node, const char *pkey, const RID *rid, PageNum left_page)
{
  int insert_pos = 0, tmp;

  for (; insert_pos < node->key_num; insert_pos++) {
    tmp = key_compare(
        file_header_.attr_type, file_header_.attr_length, pkey, node->keys + insert_pos * file_header_.key_length);
    if (tmp == 0) {
      LOG_TRACE("Insert into %d occur duplicated key, rid:%s.", file_id_, node->rids[insert_pos].to_string().c_str());
      return RC::RECORD_DUPLICATE_KEY;
    }
    if (tmp < 0)
      break;
  }

  char *from = node->keys + insert_pos * file_header_.key_length;
  char *to = from + file_header_.key_length;
  int len = (node->key_num - insert_pos) * file_header_.key_length;
  memmove(to, from, len);
  memcpy(node->keys + insert_pos * file_header_.key_length, pkey, file_header_.key_length);

  if (node->is_leaf) {
    len = (node->key_num - insert_pos) * sizeof(RID);
    memmove(node->rids + insert_pos + 1, node->rids + insert_pos, len);
    memcpy(node->rids + insert_pos, rid, sizeof(RID));

    change_leaf_parent_key_insert(node, insert_pos, left_page);
  } else {

    len = (node->key_num - insert_pos) * sizeof(RID);
    memmove(node->rids + insert_pos + 2, node->rids + insert_pos + 1, len);
    memcpy(node->rids + insert_pos + 1, rid, sizeof(RID));
  }

  node->key_num++;  //叶子结点增加一条记录
  return RC::SUCCESS;
}

RC BplusTreeHandler::split_leaf(BPPageHandle &leaf_page_handle)
{
  PageNum leaf_page;
  disk_buffer_pool_->get_page_num(&leaf_page_handle, &leaf_page);

  char *pdata;
  RC rc = disk_buffer_pool_->get_data(&leaf_page_handle, &pdata);
  if (rc != RC::SUCCESS) {
    return rc;
  }
  IndexNode *old_node = get_index_node(pdata);

  char *new_parent_key = (char *)mem_pool_item_->alloc();
  if (new_parent_key == nullptr) {
    LOG_WARN("Failed to alloc memory for new key. size=%d", file_header_.key_length);
    return RC::NOMEM;
  }

  // add a new node
  BPPageHandle page_handle2;
  rc = disk_buffer_pool_->allocate_page(file_id_, &page_handle2);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to split index page due to failed to allocate page, file_id:%d ", file_id_);
    return rc;
  }
  PageNum new_page;
  disk_buffer_pool_->get_page_num(&page_handle2, &new_page);
  disk_buffer_pool_->get_data(&page_handle2, &pdata);
  IndexNode *new_node = get_index_node(pdata);
  new_node->init_empty(file_header_);
  new_node->parent = old_node->parent;
  new_node->prev_brother = leaf_page;
  new_node->next_brother = old_node->next_brother;
  old_node->next_brother = new_page;

  // begin to move data from leaf_node to new_node
  split_node(old_node, new_node, leaf_page, new_page, new_parent_key);
  disk_buffer_pool_->mark_dirty(&leaf_page_handle);
  disk_buffer_pool_->mark_dirty(&page_handle2);

  PageNum parent_page = old_node->parent;
  rc = insert_into_parent(parent_page, leaf_page_handle, new_parent_key, page_handle2);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to insert into parent of index %d", file_id_);
    // restore status before insert into parent
    // merge_nodes function will move left node into right node
    merge_nodes(old_node, new_node, new_page, new_parent_key);
    copy_node(old_node, new_node);
    change_insert_leaf_link(old_node, new_node, leaf_page);

    mem_pool_item_->free(new_parent_key);
    disk_buffer_pool_->unpin_page(&page_handle2);
    disk_buffer_pool_->dispose_page(file_id_, new_page);
    return rc;
  }
  mem_pool_item_->free(new_parent_key);
  disk_buffer_pool_->unpin_page(&page_handle2);
  return RC::SUCCESS;
}

RC BplusTreeHandler::insert_intern_node(
    BPPageHandle &parent_page_handle, BPPageHandle &left_page_handle, BPPageHandle &right_page_handle, const char *pkey)
{
  PageNum left_page;
  disk_buffer_pool_->get_page_num(&left_page_handle, &left_page);

  PageNum right_page;
  disk_buffer_pool_->get_page_num(&right_page_handle, &right_page);

  char *pdata;
  RC rc = disk_buffer_pool_->get_data(&parent_page_handle, &pdata);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  IndexNode *node = get_index_node(pdata);

  RID rid;
  rid.page_num = right_page;
  rid.slot_num = BP_INVALID_PAGE_NUM;  // change to invalid page num

  insert_entry_into_node(node, pkey, &rid, right_page);

  disk_buffer_pool_->mark_dirty(&parent_page_handle);

  return RC::SUCCESS;
}

RC BplusTreeHandler::split_intern_node(BPPageHandle &inter_page_handle, const char *pkey)
{
  PageNum inter_page_num;
  disk_buffer_pool_->get_page_num(&inter_page_handle, &inter_page_num);

  char *pdata;
  RC rc = disk_buffer_pool_->get_data(&inter_page_handle, &pdata);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  IndexNode *inter_node = get_index_node(pdata);

  char *new_parent_key = (char *)mem_pool_item_->alloc();
  if (new_parent_key == nullptr) {
    LOG_WARN("Failed to alloc memory for new key when split intern node index %d", file_id_);
    return RC::NOMEM;
  }

  // add a new node
  BPPageHandle new_page_handle;
  rc = disk_buffer_pool_->allocate_page(file_id_, &new_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Faild to alloc new page when split inter node of index, file_id:%d", file_id_);
    mem_pool_item_->free(new_parent_key);
    return rc;
  }
  disk_buffer_pool_->get_data(&new_page_handle, &pdata);

  PageNum new_page;
  disk_buffer_pool_->get_page_num(&new_page_handle, &new_page);

  IndexNode *new_node = get_index_node(pdata);
  new_node->init_empty(file_header_);
  new_node->is_leaf = false;
  new_node->parent = inter_node->parent;

  split_node(inter_node, new_node, inter_page_num, new_page, new_parent_key);

  disk_buffer_pool_->mark_dirty(&inter_page_handle);
  disk_buffer_pool_->mark_dirty(&new_page_handle);

  // print();
  PageNum parent_page = inter_node->parent;
  rc = insert_into_parent(parent_page, inter_page_handle, new_parent_key, new_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to insert key to parents, file_id:%d", file_id_);
    merge_nodes(inter_node, new_node, new_page, new_parent_key);
    copy_node(inter_node, new_node);
    change_children_parent(inter_node->rids, inter_node->key_num + 1, inter_page_num);

    mem_pool_item_->free(new_parent_key);
    disk_buffer_pool_->unpin_page(&new_page_handle);
    disk_buffer_pool_->dispose_page(file_id_, new_page);

    return rc;
  }
  mem_pool_item_->free(new_parent_key);
  disk_buffer_pool_->unpin_page(&new_page_handle);
  return rc;
}

RC BplusTreeHandler::insert_into_parent(
    PageNum parent_page, BPPageHandle &left_page_handle, const char *pkey, BPPageHandle &right_page_handle)
{
  if (parent_page == -1) {
    return insert_into_new_root(left_page_handle, pkey, right_page_handle);
  }

  BPPageHandle page_handle;
  RC rc = disk_buffer_pool_->get_this_page(file_id_, parent_page, &page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get parent page file_id:%d, page:%d", file_id_, parent_page);
    return rc;
  }

  char *pdata;
  disk_buffer_pool_->get_data(&page_handle, &pdata);
  IndexNode *node = get_index_node(pdata);

  rc = insert_intern_node(page_handle, left_page_handle, right_page_handle, pkey);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to insert intern node of index :%d", file_id_);
    return rc;
  }
  if (node->key_num > file_header_.order) {
    rc = split_intern_node(page_handle, pkey);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to split intern node of index %d", file_id_);
      int delete_index;
      delete_entry_from_node(node, pkey, delete_index);
    }
  }

  disk_buffer_pool_->unpin_page(&page_handle);
  return rc;
}

void BplusTreeHandler::swith_root(BPPageHandle &new_root_page_handle, IndexNode *root, PageNum root_page)
{
  //@@@ TODO here should add lock

  disk_buffer_pool_->unpin_page(&root_page_handle_);
  root_page_handle_ = new_root_page_handle;
  root_node_ = root;
  file_header_.root_page = root_page;
  header_dirty_ = true;
}

/**
 * Create one new root node
 * @param left_page_handle
 * @param pkey
 * @param right_page_handle
 * @return
 */
RC BplusTreeHandler::insert_into_new_root(
    BPPageHandle &left_page_handle, const char *pkey, BPPageHandle &right_page_handle)
{
  BPPageHandle new_root_page_handle;
  RC rc = disk_buffer_pool_->allocate_page(file_id_, &new_root_page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to alloc new page for the new root node of index, file_id:%d", file_id_);
    return rc;
  }

  PageNum root_page;
  disk_buffer_pool_->get_page_num(&new_root_page_handle, &root_page);

  // modify the left node
  PageNum left_page;
  char *pdata;
  disk_buffer_pool_->get_page_num(&left_page_handle, &left_page);
  disk_buffer_pool_->get_data(&left_page_handle, &pdata);
  IndexNode *left = get_index_node(pdata);
  left->parent = root_page;
  disk_buffer_pool_->mark_dirty(&left_page_handle);

  // modify the right node
  PageNum right_page;
  disk_buffer_pool_->get_page_num(&right_page_handle, &right_page);
  disk_buffer_pool_->get_data(&right_page_handle, &pdata);
  IndexNode *right = get_index_node(pdata);
  right->parent = root_page;
  disk_buffer_pool_->mark_dirty(&right_page_handle);

  // handle the root node
  disk_buffer_pool_->get_data(&new_root_page_handle, &pdata);
  IndexNode *root = get_index_node(pdata);
  root->init_empty(file_header_);
  root->is_leaf = false;
  root->key_num = 1;
  memcpy(root->keys, pkey, file_header_.key_length);

  RID rid;
  rid.page_num = left_page;
  rid.slot_num = EMPTY_RID_SLOT_NUM;
  memcpy(root->rids, &rid, sizeof(RID));
  rid.page_num = right_page;
  rid.slot_num = EMPTY_RID_SLOT_NUM;
  memcpy(root->rids + root->key_num, &rid, sizeof(RID));

  disk_buffer_pool_->mark_dirty(&new_root_page_handle);
  swith_root(new_root_page_handle, root, root_page);

  return RC::SUCCESS;
}

RC BplusTreeHandler::insert_entry(const char *pkey, const RID *rid)
{

  if (file_id_ < 0) {
    LOG_WARN("Index isn't ready!");
    return RC::RECORD_CLOSED;
  }

  if (pkey == nullptr || rid == nullptr) {
    LOG_WARN("Invalid arguments, key is empty or rid is empty");
    return RC::INVALID_ARGUMENT;
  }

  char *key = (char *)mem_pool_item_->alloc();
  if (key == nullptr) {
    LOG_WARN("Failed to alloc memory for key. file_id:%d", file_id_);
    return RC::NOMEM;
  }
  memcpy(key, pkey, file_header_.attr_length);
  memcpy(key + file_header_.attr_length, rid, sizeof(*rid));

  PageNum leaf_page;
  RC rc = find_leaf(key, &leaf_page);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to find leaf file_id:%d, %s", file_id_, rid->to_string().c_str());
    mem_pool_item_->free(key);
    return rc;
  }

  BPPageHandle page_handle;
  rc = disk_buffer_pool_->get_this_page(file_id_, leaf_page, &page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to load leaf file_id:%d, page_num:%d", file_id_, leaf_page);
    mem_pool_item_->free(key);
    return rc;
  }

  char *pdata;
  disk_buffer_pool_->get_data(&page_handle, &pdata);

  IndexNode *leaf = get_index_node(pdata);
  rc = insert_entry_into_node(leaf, key, rid, leaf_page);
  if (rc != RC::SUCCESS) {
    LOG_TRACE("Failed to insert into leaf of index %d, rid:%s", file_id_, rid->to_string().c_str());
    disk_buffer_pool_->unpin_page(&page_handle);
    mem_pool_item_->free(key);
    return rc;
  }
  disk_buffer_pool_->mark_dirty(&page_handle);

  if (leaf->key_num > file_header_.order) {

    rc = split_leaf(page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to insert index of %d, failed to split for rid:%s", file_id_, rid->to_string().c_str());
      int delete_index = 0;
      delete_entry_from_node(leaf, key, delete_index);

      disk_buffer_pool_->unpin_page(&page_handle);
      mem_pool_item_->free(key);
      return rc;
    }
  }

  disk_buffer_pool_->unpin_page(&page_handle);
  mem_pool_item_->free(key);
  return RC::SUCCESS;
}

void BplusTreeHandler::get_entry_from_leaf(
    IndexNode *node, const char *pkey, std::list<RID> &rids, bool &continue_check)
{
  for (int i = node->key_num - 1; i >= 0; i--) {
    int tmp = attribute_comp(
        pkey, node->keys + (i * file_header_.key_length), file_header_.attr_type, file_header_.attr_length);
    if (tmp < 0) {
      if (continue_check == true) {
        LOG_WARN("Something is wrong, the sequence is wrong.");
        print_tree();
        continue_check = false;
        break;
      } else {
        continue;
      }
    } else if (tmp == 0) {
      rids.push_back(node->rids[i]);
      continue_check = true;
    } else {
      continue_check = false;
      break;
    }
  }
}

RC BplusTreeHandler::get_entry(const char *pkey, std::list<RID> &rids)
{
  if (file_id_ < 0) {
    LOG_WARN("Index isn't ready!");
    return RC::RECORD_CLOSED;
  }

  char *key = (char *)mem_pool_item_->alloc();
  if (key == nullptr) {
    LOG_WARN("Failed to alloc memory for key. size=%d", file_header_.key_length);
    return RC::NOMEM;
  }
  memcpy(key, pkey, file_header_.attr_length);

  RC rc;

  BPPageHandle page_handle;
  char *pdata;
  IndexNode *node = root_node_;
  while (false == node->is_leaf) {

    int i;
    for (i = 0; i < node->key_num; i++) {
      int tmp = attribute_comp(
          pkey, node->keys + i * file_header_.key_length, file_header_.attr_type, file_header_.attr_length);
      if (tmp < 0)
        break;
    }

    if (page_handle.open == true) {
      disk_buffer_pool_->unpin_page(&page_handle);
    }

    rc = disk_buffer_pool_->get_this_page(file_id_, node->rids[i].page_num, &page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to load page file_id:%d, page_num:%d", file_id_, node->rids[i].page_num);
      return rc;
    }
    disk_buffer_pool_->get_data(&page_handle, &pdata);

    node = get_index_node(pdata);
  }

  bool continue_check = false;
  get_entry_from_leaf(node, key, rids, continue_check);

  while (continue_check == true) {
    PageNum prev_brother = node->prev_brother;
    if (prev_brother == EMPTY_RID_PAGE_NUM) {
      break;
    }
    if (page_handle.open) {
      disk_buffer_pool_->unpin_page(&page_handle);
    }

    rc = disk_buffer_pool_->get_this_page(file_id_, prev_brother, &page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Skip load the previous page, file_id:%d", file_id_);
      break;
    }
    disk_buffer_pool_->get_data(&page_handle, &pdata);
    node = get_index_node(pdata);

    get_entry_from_leaf(node, key, rids, continue_check);
  }

  if (page_handle.open) {
    disk_buffer_pool_->unpin_page(&page_handle);
  }
  mem_pool_item_->free(key);
  return RC::SUCCESS;
}

void BplusTreeHandler::delete_entry_from_node(IndexNode *node, const int delete_index)
{
  char *from = node->keys + (delete_index + 1) * file_header_.key_length;
  char *to = from - file_header_.key_length;
  int len = (node->key_num - delete_index - 1) * file_header_.key_length;
  memmove(to, from, len);

  RID *from_rid = node->rids + (delete_index + 1);
  RID *to_rid = from_rid - 1;
  len = (node->key_num - delete_index - 1) * sizeof(RID);
  if (node->is_leaf == false) {
    len += sizeof(RID);
  }
  memmove(to_rid, from_rid, len);

  node->key_num--;
}

RC BplusTreeHandler::get_parent_changed_index(
    BPPageHandle &parent_handle, IndexNode *&parent, IndexNode *node, PageNum page_num, int &changed_index)
{
  RC rc = disk_buffer_pool_->get_this_page(file_id_, node->parent, &parent_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to delete index, due to failed to get pareent page, file_id:%d, parent_page:%d",
        file_id_,
        node->parent);
    return rc;
  }
  char *pdata;
  disk_buffer_pool_->get_data(&parent_handle, &pdata);
  parent = get_index_node(pdata);

  while (changed_index <= parent->key_num) {
    if ((parent->rids[changed_index].page_num) == page_num)
      break;
    changed_index++;
  }

  if (changed_index == parent->key_num + 1) {
    LOG_WARN("Something is wrong, failed to find the target page %d in parent, node:%s file_id:%d",
        page_num,
        node->to_string(file_header_).c_str(),
        file_id_);
    print_tree();
    return RC::RECORD_CLOSED;
  }

  return RC::SUCCESS;
}

RC BplusTreeHandler::change_leaf_parent_key_insert(IndexNode *node, int changed_indx, PageNum page_num)
{
  if (changed_indx != 0) {
    return RC::SUCCESS;
  }

  if (node->is_leaf == false) {
    return RC::SUCCESS;
  }

  if (node->parent == -1) {
    return RC::SUCCESS;
  }

  if (node->key_num == 0) {
    return RC::SUCCESS;
  }

  if (node->prev_brother == -1) {
    return RC::SUCCESS;
  }

  int parent_changed_index = 0;
  BPPageHandle parent_handle;
  IndexNode *parent = nullptr;

  RC rc = get_parent_changed_index(parent_handle, parent, node, page_num, parent_changed_index);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get parent's delete index, file_id:%d, child's page_num:%d", file_id_, page_num);
    if (parent_handle.open) {
      disk_buffer_pool_->unpin_page(&parent_handle);
      return rc;
    }
  }
  if (parent_changed_index > 0) {
    memcpy(parent->keys + (parent_changed_index - 1) * file_header_.key_length, node->keys, file_header_.key_length);
  }

  disk_buffer_pool_->unpin_page(&parent_handle);
  return RC::SUCCESS;
}
RC BplusTreeHandler::change_leaf_parent_key_delete(IndexNode *leaf, int delete_indx, const char *old_first_key)
{
  if (delete_indx != 0) {
    return RC::SUCCESS;
  }

  if (leaf->is_leaf == false) {
    return RC::SUCCESS;
  }

  if (leaf->parent == -1) {
    return RC::SUCCESS;
  }

  if (leaf->prev_brother == -1) {
    return RC::SUCCESS;
  }

  if (leaf->key_num == 0) {
    return RC::SUCCESS;
  }

  IndexNode *node = leaf;
  bool found = false;
  while (node->parent != -1) {
    int index = 0;
    BPPageHandle parent_handle;

    RC rc = disk_buffer_pool_->get_this_page(file_id_, node->parent, &parent_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to delete index, due to failed to get pareent page, file_id:%d, parent_page:%d",
          file_id_,
          node->parent);
      return rc;
    }
    char *pdata;
    disk_buffer_pool_->get_data(&parent_handle, &pdata);
    node = get_index_node(pdata);

    int tmp = 0;
    while (index < node->key_num) {
      tmp = key_compare(file_header_.attr_type,
          file_header_.attr_length,
          old_first_key,
          node->keys + index * file_header_.key_length);
      if (tmp == 0) {
        found = true;
        memcpy(node->keys + index * file_header_.key_length, leaf->keys, file_header_.key_length);
        break;
      } else if (tmp > 0) {
        index++;
        continue;
      } else {
        break;
      }
    }

    disk_buffer_pool_->unpin_page(&parent_handle);

    if (found == true) {
      return RC::SUCCESS;
    }
  }

  if (found == false) {
    LOG_INFO("The old fist key has been changed, leaf:%s", leaf->to_string(file_header_).c_str());
    print_tree();
  }
  return RC::SUCCESS;
}
RC BplusTreeHandler::delete_entry_from_node(IndexNode *node, const char *pkey, int &node_delete_index)
{

  int delete_index, tmp;
  for (delete_index = 0; delete_index < node->key_num; delete_index++) {
    tmp = key_compare(
        file_header_.attr_type, file_header_.attr_length, pkey, node->keys + delete_index * file_header_.key_length);
    if (tmp == 0) {
      node_delete_index = delete_index;
      break;
    }
  }
  if (delete_index >= node->key_num) {
    // LOG_WARN("Failed to delete index of %d", file_id_);
    return RC::RECORD_INVALID_KEY;
  }

  delete_entry_from_node(node, delete_index);

  // change parent's key
  change_leaf_parent_key_delete(node, delete_index, pkey);
  return RC::SUCCESS;
}

RC BplusTreeHandler::change_insert_leaf_link(IndexNode *left, IndexNode *right, PageNum right_page)
{
  if (left->is_leaf == false) {
    return RC::SUCCESS;
  }

  if (right->next_brother != -1) {
    PageNum next_right_page = right->next_brother;
    BPPageHandle next_right_handle;
    RC rc = disk_buffer_pool_->get_this_page(file_id_, next_right_page, &next_right_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to set link for leaf for node %s, file_id:%d", file_id_, right->to_string(file_header_).c_str());
      return rc;
    }

    char *pdata;
    disk_buffer_pool_->get_data(&next_right_handle, &pdata);
    IndexNode *next_right = get_index_node(pdata);
    next_right->prev_brother = right_page;
    disk_buffer_pool_->mark_dirty(&next_right_handle);
    disk_buffer_pool_->unpin_page(&next_right_handle);
  }

  return RC::SUCCESS;
}

RC BplusTreeHandler::change_delete_leaf_link(IndexNode *left, IndexNode *right, PageNum right_page)
{
  if (left->is_leaf == false) {
    return RC::SUCCESS;
  }

  right->prev_brother = left->prev_brother;
  if (left->prev_brother != -1) {
    PageNum prev_left_page = left->prev_brother;
    BPPageHandle prev_left_handle;
    RC rc = disk_buffer_pool_->get_this_page(file_id_, prev_left_page, &prev_left_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to set link for leaf for node %s, file_id:%d", file_id_, right->to_string(file_header_).c_str());
      return rc;
    }

    char *pdata;
    disk_buffer_pool_->get_data(&prev_left_handle, &pdata);
    IndexNode *prev_left = get_index_node(pdata);
    prev_left->next_brother = right_page;
    disk_buffer_pool_->mark_dirty(&prev_left_handle);
    disk_buffer_pool_->unpin_page(&prev_left_handle);
  }

  return RC::SUCCESS;
}

/**
 * merge left node into right node.
 * @param parent_handle
 * @param left_handle
 * @param right_handle
 * @param delete_index
 * @return
 */
RC BplusTreeHandler::coalesce_node(BPPageHandle &parent_handle, BPPageHandle &left_handle, BPPageHandle &right_handle,
    int delete_index, bool check_change_leaf_key, int node_delete_index, const char *pkey)
{
  PageNum left_page, right_page, parent_page;
  IndexNode *left, *right, *parent;
  char *pdata, *parent_key;
  RC rc;

  disk_buffer_pool_->get_page_num(&left_handle, &left_page);
  disk_buffer_pool_->get_data(&left_handle, &pdata);
  left = get_index_node(pdata);

  disk_buffer_pool_->get_page_num(&right_handle, &right_page);
  disk_buffer_pool_->get_data(&right_handle, &pdata);
  right = get_index_node(pdata);

  parent_page = left->parent;
  disk_buffer_pool_->get_data(&parent_handle, &pdata);
  parent = get_index_node(pdata);

  parent_key = (char *)mem_pool_item_->alloc();
  if (parent_key == nullptr) {
    LOG_WARN("Failed to alloc memory for key. size=%d", file_header_.key_length);
    return RC::NOMEM;
  }

  memcpy(parent_key, parent->keys + delete_index * file_header_.key_length, file_header_.key_length);
  merge_nodes(left, right, right_page, parent_key);
  disk_buffer_pool_->mark_dirty(&left_handle);
  disk_buffer_pool_->mark_dirty(&right_handle);

  change_delete_leaf_link(left, right, right_page);
  if (check_change_leaf_key) {
    change_leaf_parent_key_delete(right, node_delete_index, pkey);
  }

  rc = delete_entry_internal(parent_page, parent_key);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to delete internal entry of index ", file_id_);

    // restore status
    copy_node(left, right);
    right->key_num = 0;
    split_node(left, right, left_page, right_page, parent_key);
    change_delete_leaf_link(left, right, left_page);
    left->next_brother = right_page;
    right->prev_brother = left_page;

    mem_pool_item_->free(parent_key);
    return rc;
  }

  mem_pool_item_->free(parent_key);
  return RC::SUCCESS;
}

void BplusTreeHandler::change_children_parent(RID *rids, int rid_len, PageNum new_parent_page)
{
  for (int i = 0; i < rid_len; i++) {
    RID rid = rids[i];

    PageNum page_num = rid.page_num;

    BPPageHandle child_page_handle;
    RC rc = disk_buffer_pool_->get_this_page(file_id_, page_num, &child_page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to load child page %d of index %d when change child's parent.", file_id_, page_num);
      continue;
    }

    char *pdata;
    disk_buffer_pool_->get_data(&child_page_handle, &pdata);

    IndexNode *child_node = get_index_node(pdata);
    child_node->parent = new_parent_page;

    disk_buffer_pool_->mark_dirty(&child_page_handle);
    disk_buffer_pool_->unpin_page(&child_page_handle);
  }
}

/**
 * merge left node into right node;
 *
 * This function is contrary to split_node
 */
void BplusTreeHandler::merge_nodes(IndexNode *left_node, IndexNode *right_node, PageNum right_page, char *parent_key)
{
  bool is_leaf = left_node->is_leaf;
  int old_left_key_num = left_node->key_num;
  int old_right_key_num = right_node->key_num;
  int new_left_key_num = 0;
  int new_right_key_num = old_left_key_num + old_right_key_num;
  if (is_leaf == false) {
    new_right_key_num++;
  }
  left_node->key_num = new_left_key_num;
  right_node->key_num = new_right_key_num;

  if (is_leaf) {
    int delta = new_right_key_num - old_right_key_num;
    char *from = right_node->keys;
    char *to = right_node->keys + delta * file_header_.key_length;
    int len = old_right_key_num * file_header_.key_length;
    memmove(to, from, len);

    RID *from_rid = right_node->rids;
    RID *to_rid = right_node->rids + delta;
    len = old_right_key_num * sizeof(RID);
    memmove(to_rid, from_rid, len);

    from = left_node->keys;
    to = right_node->keys;
    len = old_left_key_num * file_header_.key_length;
    memmove(to, from, len);

    from_rid = left_node->rids;
    to_rid = right_node->rids;
    len = old_left_key_num * sizeof(RID);
    memmove(to_rid, from_rid, len);

  } else {
    int delta = new_right_key_num - old_right_key_num;
    char *from = right_node->keys;
    char *to = right_node->keys + delta * file_header_.key_length;
    int len = old_right_key_num * file_header_.key_length;
    memmove(to, from, len);

    RID *from_rid = right_node->rids;
    RID *to_rid = right_node->rids + delta;
    len = (old_right_key_num + 1) * sizeof(RID);
    memmove(to_rid, from_rid, len);

    memcpy(right_node->keys + (delta - 1) * file_header_.key_length, parent_key, file_header_.key_length);

    from = left_node->keys;
    to = right_node->keys;
    len = old_left_key_num * file_header_.key_length;
    memmove(to, from, len);

    from_rid = left_node->rids;
    to_rid = right_node->rids;
    len = (old_left_key_num + 1) * sizeof(RID);
    memmove(to_rid, from_rid, len);

    change_children_parent(to_rid, len / sizeof(RID), right_page);
  }
}

/**
 * split left node to two node
 * This function is contrary to merge_node
 */
void BplusTreeHandler::split_node(
    IndexNode *left_node, IndexNode *right_node, PageNum left_page, PageNum right_page, char *new_parent_key)
{
  bool is_leaf = left_node->is_leaf;
  int old_left_key_num = left_node->key_num;
  int old_right_key_num = right_node->key_num;  // right_node->key_num should be zero
  int total_key_num = left_node->key_num + right_node->key_num;

  int mid, new_left_key_num, new_right_key_num;

  /**
   * if node is leaf, all key will be distributed both in left and right node
   * if node is intern node, all keys except the middle key will be distributed both in the left and the right node
   */
  if (is_leaf == true) {
    new_left_key_num = total_key_num / 2;
    mid = new_left_key_num;
    new_right_key_num = total_key_num - mid;
  } else {
    new_left_key_num = (total_key_num - 1) / 2;
    mid = new_left_key_num + 1;
    new_right_key_num = (total_key_num - mid);
  }

  left_node->key_num = new_left_key_num;
  right_node->key_num = new_right_key_num;

  if (is_leaf) {
    memcpy(new_parent_key, left_node->keys + new_left_key_num * file_header_.key_length, file_header_.key_length);
  } else {
    memmove(new_parent_key, left_node->keys + new_left_key_num * file_header_.key_length, file_header_.key_length);
  }

  char *from = left_node->keys + mid * file_header_.key_length;
  char *to = right_node->keys;
  int len = new_right_key_num * file_header_.key_length;
  memmove(to, from, len);

  RID *from_rid = left_node->rids + mid;
  RID *to_rid = right_node->rids;
  len = new_right_key_num * sizeof(RID);
  memmove(to_rid, from_rid, len);

  // handle the last rid
  if (is_leaf == false) {

    RID *changed_rids = to_rid;
    int changed_rids_len = len;
    PageNum changed_page = right_page;

    if (old_right_key_num == 0) {
      memmove(right_node->rids + new_right_key_num, left_node->rids + old_left_key_num, sizeof(RID));
      changed_rids_len += sizeof(RID);
    }

    change_children_parent(changed_rids, changed_rids_len / sizeof(RID), changed_page);
  } else {
    change_insert_leaf_link(left_node, right_node, right_page);
  }

  return;
}

void BplusTreeHandler::copy_node(IndexNode *to, IndexNode *from)
{
  memcpy(to->keys, from->keys, from->key_num * file_header_.key_length);
  memcpy(to->rids, from->rids, (from->key_num + 1) * sizeof(RID));
  memcpy(to, from, sizeof(IndexNode));
}

void BplusTreeHandler::redistribute_nodes(
    IndexNode *left_node, IndexNode *right_node, PageNum left_page, PageNum right_page, char *parent_key)
{
  bool is_leaf = left_node->is_leaf;
  int old_left_key_num = left_node->key_num;
  int old_right_key_num = right_node->key_num;
  int total_key_num = left_node->key_num + right_node->key_num;
  if (is_leaf == false) {
    total_key_num++;
  }

  // mid represent the parent key's position
  int mid, new_left_key_num, new_right_key_num;

  /**
   * if node is leaf, all key will be distributed both in left and right node
   * if node is intern node, all keys except the middle key will be distributed both in the left and the right node
   */
  if (is_leaf == true) {
    new_left_key_num = total_key_num / 2;
    mid = new_left_key_num;
    new_right_key_num = total_key_num - mid;
  } else {
    new_left_key_num = (total_key_num - 1) / 2;
    mid = new_left_key_num + 1;
    new_right_key_num = (total_key_num - mid);
  }

  left_node->key_num = new_left_key_num;
  right_node->key_num = new_right_key_num;

  RID *changed_rids = nullptr;
  int changed_rids_len = 0;
  PageNum changed_page = 0;

  int delta = old_left_key_num - new_left_key_num;
  if (delta == 0) {
    return;
  } else if (delta > 0) {
    // move kv from left to right
    delta = new_right_key_num - old_right_key_num;
    char *from = right_node->keys;
    char *to = right_node->keys + delta * file_header_.key_length;
    int len = old_right_key_num * file_header_.key_length;
    memmove(to, from, len);

    RID *from_rid = right_node->rids;
    RID *to_rid = right_node->rids + delta;
    len = old_right_key_num * sizeof(RID);
    if (left_node->is_leaf == false) {
      len += sizeof(RID);
    }
    memmove(to_rid, from_rid, len);

    if (is_leaf == false) {
      memcpy(left_node->keys + old_left_key_num * file_header_.key_length, parent_key, file_header_.key_length);
    }

    delta = old_left_key_num - new_left_key_num;

    from = left_node->keys + mid * file_header_.key_length;
    to = right_node->keys;
    len = delta * file_header_.key_length;
    memmove(to, from, len);

    from_rid = left_node->rids + mid;
    to_rid = right_node->rids;
    len = delta * sizeof(RID);
    memmove(to_rid, from_rid, len);

    changed_rids = to_rid;
    changed_rids_len = len;
    changed_page = right_page;

    if (is_leaf) {
      memcpy(parent_key, right_node->keys, file_header_.key_length);
    } else {
      memmove(parent_key, left_node->keys + new_left_key_num * file_header_.key_length, file_header_.key_length);
    }

  } else {
    // move kv from right to left
    if (is_leaf == false) {
      memcpy(left_node->keys + old_left_key_num * file_header_.key_length, parent_key, file_header_.key_length);
    }
    int start_pos = old_left_key_num;
    int len = (new_left_key_num - old_left_key_num);
    if (is_leaf == false) {
      start_pos++;
      len--;
    }

    char *from = right_node->keys;
    char *to = left_node->keys + start_pos * file_header_.key_length;
    memmove(to, from, len * file_header_.key_length);

    RID *from_rid = right_node->rids;
    RID *to_rid = left_node->rids + start_pos;
    memmove(to_rid, from_rid, len * sizeof(RID));

    changed_rids = to_rid;
    changed_rids_len = (new_left_key_num - old_left_key_num) * sizeof(RID);
    changed_page = left_page;

    if (is_leaf == false) {
      memcpy(parent_key, right_node->keys + len * file_header_.key_length, file_header_.key_length);
      memcpy(left_node->rids + new_left_key_num, right_node->rids + len, sizeof(RID));
    } else {
      memcpy(parent_key, right_node->keys + len * file_header_.key_length, file_header_.key_length);
    }

    delta = old_right_key_num - new_right_key_num;
    from = right_node->keys + delta * file_header_.key_length;
    to = right_node->keys;
    len = new_right_key_num * file_header_.key_length;
    memmove(to, from, len);

    from_rid = right_node->rids + delta;
    to_rid = right_node->rids;
    len = new_right_key_num * sizeof(RID);
    if (left_node->is_leaf == false) {
      len += sizeof(RID);
    }
    memmove(to_rid, from_rid, len);
  }

  // handle the last rid
  if (left_node->is_leaf == false) {
    change_children_parent(changed_rids, changed_rids_len / sizeof(RID), changed_page);
  }

  return;
}

RC BplusTreeHandler::redistribute_nodes(
    BPPageHandle &parent_handle, BPPageHandle &left_handle, BPPageHandle &right_handle)
{

  PageNum left_page, right_page;
  IndexNode *left, *right, *parent;
  char *pdata;

  disk_buffer_pool_->get_page_num(&left_handle, &left_page);
  disk_buffer_pool_->get_data(&left_handle, &pdata);
  left = get_index_node(pdata);

  disk_buffer_pool_->get_page_num(&right_handle, &right_page);
  disk_buffer_pool_->get_data(&right_handle, &pdata);
  right = get_index_node(pdata);

  disk_buffer_pool_->get_data(&parent_handle, &pdata);
  parent = get_index_node(pdata);

  int parent_change_pos = -1;
  for (int k = 0; k < parent->key_num; k++) {

    if (parent->rids[k].page_num == left_page) {
      parent_change_pos = k;
      break;
    }
  }

  if (parent_change_pos == -1) {
    LOG_WARN("Failed to find the parent pos during redistribute node");
    return RC::RECORD_INVALID_KEY;
  }

  char *parent_key = parent->keys + parent_change_pos * file_header_.key_length;
  redistribute_nodes(left, right, left_page, right_page, parent_key);

  disk_buffer_pool_->mark_dirty(&left_handle);
  disk_buffer_pool_->mark_dirty(&right_handle);
  disk_buffer_pool_->mark_dirty(&parent_handle);

  return RC::SUCCESS;
}

RC BplusTreeHandler::clean_root_after_delete(IndexNode *old_root)
{
  if (old_root->key_num > 0) {
    return RC::SUCCESS;
  }

  BPPageHandle root_handle;
  RC rc = disk_buffer_pool_->get_this_page(file_id_, old_root->rids[0].page_num, &root_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get new root page %d of index %d", old_root->rids[0].page_num, file_id_);
    return rc;
  }

  char *pdata;
  disk_buffer_pool_->get_data(&root_handle, &pdata);
  IndexNode *root = get_index_node(pdata);
  root->parent = -1;
  disk_buffer_pool_->mark_dirty(&root_handle);
  swith_root(root_handle, root, old_root->rids[0].page_num);

  return RC::SUCCESS;
}

RC BplusTreeHandler::can_merge_with_other(BPPageHandle *page_handle, PageNum page_num, bool *can_merge)
{
  RC rc = disk_buffer_pool_->get_this_page(file_id_, page_num, page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to delete index, due to failed to get page of current delete page, file_id:%d, page:%d",
        file_id_,
        page_num);
    return rc;
  }
  char *pdata;
  disk_buffer_pool_->get_data(page_handle, &pdata);
  IndexNode *node = get_index_node(pdata);
  *can_merge = node->key_num > (file_header_.order / 2);

  return RC::SUCCESS;
}

RC BplusTreeHandler::delete_entry_internal(PageNum page_num, const char *pkey)
{
  BPPageHandle page_handle;
  RC rc = disk_buffer_pool_->get_this_page(file_id_, page_num, &page_handle);
  if (rc != RC::SUCCESS) {
    LOG_WARN(
        "Failed to delete entry in index node, due to failed to get page!, file_id:%d, page:%d", file_id_, page_num);
    return rc;
  }

  char *pdata;
  rc = disk_buffer_pool_->get_data(&page_handle, &pdata);
  if (rc != RC::SUCCESS) {
    return rc;
  }
  IndexNode *node = get_index_node(pdata);

  int node_delete_index = -1;
  rc = delete_entry_from_node(node, pkey, node_delete_index);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to delete index %d", file_id_);
    return rc;
  }

  disk_buffer_pool_->mark_dirty(&page_handle);

  int min_key = file_header_.order / 2;
  if (node->key_num >= min_key) {
    disk_buffer_pool_->unpin_page(&page_handle);
    return RC::SUCCESS;
  }

  if (node->parent == -1) {
    if (node->key_num == 0 && node->is_leaf == false) {
      rc = clean_root_after_delete(node);
      if (rc != RC::SUCCESS) {
        LOG_WARN("Failed to clean root after delete all entry in the root, file_id:%d", file_id_);
        insert_entry_into_node(node, pkey, (RID *)(pkey + file_header_.attr_length), page_num);
        disk_buffer_pool_->unpin_page(&page_handle);
        return rc;
      }
      disk_buffer_pool_->unpin_page(&page_handle);
      disk_buffer_pool_->dispose_page(file_id_, page_num);

      return RC::SUCCESS;
    }

    disk_buffer_pool_->unpin_page(&page_handle);
    return RC::SUCCESS;
  }

  int delete_index = 0;
  BPPageHandle parent_handle;
  IndexNode *parent = nullptr;

  rc = get_parent_changed_index(parent_handle, parent, node, page_num, delete_index);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to get parent delete index");
    insert_entry_into_node(node, pkey, (RID *)(pkey + file_header_.attr_length), page_num);
    disk_buffer_pool_->unpin_page(&page_handle);
    return rc;
  }

  bool can_merge_with_right = false;
  bool force_collapse_with_right = false;
  bool can_merge_with_left = false;
  // bool force_collapse_with_left = false;
  PageNum left_page = 0, right_page = 0;
  BPPageHandle right_handle, left_handle;
  if (delete_index == 0) {
    right_page = parent->rids[delete_index + 1].page_num;
    rc = can_merge_with_other(&right_handle, right_page, &can_merge_with_right);
    if (rc != RC::SUCCESS) {
      goto cleanup;
    }

    if (can_merge_with_right == false) {
      force_collapse_with_right = true;
    }
  } else {
    left_page = parent->rids[delete_index - 1].page_num;
    rc = can_merge_with_other(&left_handle, left_page, &can_merge_with_left);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed delete index, due to failed to get page, file_id:%d, page:%d", file_id_, left_page);
      goto cleanup;
    }
    if (can_merge_with_left == false) {
      // begin to merge with right
      // force_collapse_with_left = true;
      if (delete_index < parent->key_num) {

        right_page = parent->rids[delete_index + 1].page_num;
        rc = can_merge_with_other(&right_handle, right_page, &can_merge_with_right);
        if (rc != RC::SUCCESS) {
          LOG_WARN("Failed to delete index, due to failed to get right page of current delete page, file_id:%d, "
                   "right_page:$d",
              file_id_,
              right_page);

          goto cleanup;
        }

      }  // delete_index < parent->key_num - 1
    }    // if can_merge_with_left = false
  }      // delete_index = 0

  if (can_merge_with_left) {
    rc = redistribute_nodes(parent_handle, left_handle, page_handle);
  } else if (can_merge_with_right) {
    rc = redistribute_nodes(parent_handle, page_handle, right_handle);
    change_leaf_parent_key_delete(node, node_delete_index, pkey);
  } else if (force_collapse_with_right) {
    rc = coalesce_node(parent_handle, page_handle, right_handle, delete_index, true, node_delete_index, pkey);
    if (rc == RC::SUCCESS) {
      disk_buffer_pool_->unpin_page(&page_handle);
      disk_buffer_pool_->dispose_page(file_id_, page_num);
      page_handle.open = false;
    }
  } else {
    rc = coalesce_node(parent_handle, left_handle, page_handle, delete_index - 1, false, node_delete_index, pkey);
    if (rc == RC::SUCCESS) {
      disk_buffer_pool_->unpin_page(&left_handle);
      disk_buffer_pool_->dispose_page(file_id_, left_page);
      left_handle.open = false;
    }
  }

cleanup:
  if (rc != RC::SUCCESS) {
    insert_entry_into_node(node, pkey, (RID *)(pkey + file_header_.attr_length), page_num);
  }
  if (right_handle.open) {
    disk_buffer_pool_->unpin_page(&right_handle);
  }
  if (left_handle.open) {
    disk_buffer_pool_->unpin_page(&left_handle);
  }
  disk_buffer_pool_->unpin_page(&parent_handle);
  if (page_handle.open) {
    disk_buffer_pool_->unpin_page(&page_handle);
  }

  return rc;
}

RC BplusTreeHandler::delete_entry(const char *data, const RID *rid)
{
  if (file_id_ < 0) {
    LOG_WARN("Failed to delete index entry, due to index is't ready");
    return RC::RECORD_CLOSED;
  }

  char *pkey = (char *)mem_pool_item_->alloc();
  if (nullptr == pkey) {
    LOG_WARN("Failed to alloc memory for key. size=%d", file_header_.key_length);
    return RC::NOMEM;
  }
  memcpy(pkey, data, file_header_.attr_length);
  memcpy(pkey + file_header_.attr_length, rid, sizeof(*rid));

  PageNum leaf_page;
  RC rc = find_leaf(pkey, &leaf_page);
  if (rc != RC::SUCCESS) {
    mem_pool_item_->free(pkey);
    return rc;
  }
  rc = delete_entry_internal(leaf_page, pkey);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to delete index %d", file_id_);
    mem_pool_item_->free(pkey);
    return rc;
  }
  mem_pool_item_->free(pkey);
  return RC::SUCCESS;
}

RC BplusTreeHandler::find_first_index_satisfied(CompOp compop, const char *key, PageNum *page_num, int *rididx)
{
  BPPageHandle page_handle;
  IndexNode *node;
  PageNum leaf_page, next;
  char *pdata, *pkey;
  RC rc;
  int i, tmp;
  RID rid;
  if (compop == LESS_THAN || compop == LESS_EQUAL || compop == NOT_EQUAL) {
    rc = get_first_leaf_page(page_num);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to get first leaf page, index:%d", file_id_);
      return rc;
    }
    *rididx = 0;
    return RC::SUCCESS;
  }
  rid.page_num = -1;
  rid.slot_num = -1;
  pkey = (char *)mem_pool_item_->alloc();
  if (pkey == nullptr) {
    LOG_WARN("Failed to alloc memory for key. size=%d", file_header_.key_length);
    return RC::NOMEM;
  }
  memcpy(pkey, key, file_header_.attr_length);
  memcpy(pkey + file_header_.attr_length, &rid, sizeof(RID));

  rc = find_leaf(pkey, &leaf_page);
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to find leaf page of index %d", file_id_);
    mem_pool_item_->free(pkey);
    return rc;
  }
  mem_pool_item_->free(pkey);

  next = leaf_page;

  while (next > 0) {
    rc = disk_buffer_pool_->get_this_page(file_id_, next, &page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to scan index due to failed to load page %d of index %d", next, file_id_);
      return rc;
    }
    disk_buffer_pool_->get_data(&page_handle, &pdata);

    node = get_index_node(pdata);
    for (i = 0; i < node->key_num; i++) {
      tmp = attribute_comp(
          node->keys + i * file_header_.key_length, key, file_header_.attr_type, file_header_.attr_length);
      if (compop == EQUAL_TO || compop == GREAT_EQUAL) {
        if (tmp >= 0) {
          disk_buffer_pool_->get_page_num(&page_handle, page_num);

          *rididx = i;
          disk_buffer_pool_->unpin_page(&page_handle);
          return RC::SUCCESS;
        }
      }
      if (compop == GREAT_THAN) {
        if (tmp > 0) {
          disk_buffer_pool_->get_page_num(&page_handle, page_num);
          *rididx = i;
          disk_buffer_pool_->unpin_page(&page_handle);
          return RC::SUCCESS;
        }
      }
    }
    next = node->next_brother;
  }
  disk_buffer_pool_->unpin_page(&page_handle);

  return RC::RECORD_EOF;
}

RC BplusTreeHandler::get_first_leaf_page(PageNum *leaf_page)
{
  RC rc;
  BPPageHandle page_handle;
  PageNum page_num;
  IndexNode *node;
  char *pdata;

  node = root_node_;

  while (node->is_leaf == false) {
    page_num = node->rids[0].page_num;
    if (page_handle.open) {
      disk_buffer_pool_->unpin_page(&page_handle);
    }

    rc = disk_buffer_pool_->get_this_page(file_id_, page_num, &page_handle);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to load page %d of index %d", page_num, file_id_);
      return rc;
    }
    disk_buffer_pool_->get_data(&page_handle, &pdata);

    node = get_index_node(pdata);
  }
  if (page_handle.open) {
    disk_buffer_pool_->get_page_num(&page_handle, leaf_page);

    disk_buffer_pool_->unpin_page(&page_handle);
  } else {
    disk_buffer_pool_->get_page_num(&root_page_handle_, leaf_page);
  }

  return RC::SUCCESS;
}

BplusTreeScanner::BplusTreeScanner(BplusTreeHandler &index_handler) : index_handler_(index_handler)
{}

RC BplusTreeScanner::open(CompOp comp_op, const char *value)
{
  RC rc;
  if (opened_) {
    return RC::RECORD_OPENNED;
  }

  comp_op_ = comp_op;

  char *value_copy = (char *)malloc(index_handler_.file_header_.attr_length);
  if (value_copy == nullptr) {
    LOG_WARN("Failed to alloc memory for value. size=%d", index_handler_.file_header_.attr_length);
    return RC::NOMEM;
  }
  memcpy(value_copy, value, index_handler_.file_header_.attr_length);
  value_ = value_copy;  // mem_pool_item_->free value_
  rc = index_handler_.find_first_index_satisfied(comp_op, value, &next_page_num_, &index_in_node_);
  if (rc != RC::SUCCESS) {
    if (rc == RC::RECORD_EOF) {
      next_page_num_ = -1;
      index_in_node_ = -1;
    } else
      return rc;
  }
  num_fixed_pages_ = 1;
  next_index_of_page_handle_ = 0;
  pinned_page_count_ = 0;
  opened_ = true;
  return RC::SUCCESS;
}

RC BplusTreeScanner::close()
{
  if (!opened_) {
    return RC::RECORD_SCANCLOSED;
  }
  free((void *)value_);
  value_ = nullptr;
  opened_ = false;
  return RC::SUCCESS;
}

RC BplusTreeScanner::next_entry(RID *rid)
{
  RC rc;
  if (!opened_) {
    return RC::RECORD_CLOSED;
  }
  rc = get_next_idx_in_memory(rid);  //和RM中一样，有可能有错误，一次只查当前页和当前页的下一页，有待确定
  if (rc == RC::RECORD_NO_MORE_IDX_IN_MEM) {
    rc = find_idx_pages();
    if (rc != RC::SUCCESS) {
      return rc;
    }
    return get_next_idx_in_memory(rid);
  } else {
    if (rc != RC::SUCCESS) {
      return rc;
    }
  }
  return RC::SUCCESS;
}

RC BplusTreeScanner::find_idx_pages()
{
  RC rc;
  if (!opened_) {
    return RC::RECORD_CLOSED;
  }
  if (pinned_page_count_ > 0) {
    for (int i = 0; i < pinned_page_count_; i++) {
      rc = index_handler_.disk_buffer_pool_->unpin_page(page_handles_ + i);
      if (rc != RC::SUCCESS) {
        return rc;
      }
    }
  }
  next_index_of_page_handle_ = 0;
  pinned_page_count_ = 0;

  for (int i = 0; i < num_fixed_pages_; i++) {
    if (next_page_num_ <= 0)
      break;
    rc = index_handler_.disk_buffer_pool_->get_this_page(index_handler_.file_id_, next_page_num_, page_handles_ + i);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    char *pdata;
    rc = index_handler_.disk_buffer_pool_->get_data(page_handles_ + i, &pdata);
    if (rc != RC::SUCCESS) {
      return rc;
    }

    IndexNode *node = index_handler_.get_index_node(pdata);
    pinned_page_count_++;
    next_page_num_ = node->next_brother;
  }
  if (pinned_page_count_ > 0)
    return RC::SUCCESS;
  return RC::RECORD_EOF;
}

RC BplusTreeScanner::get_next_idx_in_memory(RID *rid)
{
  char *pdata;
  IndexNode *node;
  RC rc;
  if (next_index_of_page_handle_ >= pinned_page_count_) {
    return RC::RECORD_NO_MORE_IDX_IN_MEM;
  }

  if (next_page_num_ == -1 && index_in_node_ == -1) {
    return RC::RECORD_EOF;
  }

  for (; next_index_of_page_handle_ < pinned_page_count_; next_index_of_page_handle_++) {
    rc = index_handler_.disk_buffer_pool_->get_data(page_handles_ + next_index_of_page_handle_, &pdata);
    if (rc != RC::SUCCESS) {
      LOG_WARN("Failed to get data from disk buffer pool. rc=%s", strrc);
      return rc;
    }

    node = index_handler_.get_index_node(pdata);
    for (; index_in_node_ < node->key_num; index_in_node_++) {
      if (satisfy_condition(node->keys + index_in_node_ * index_handler_.file_header_.key_length)) {
        memcpy(rid, node->rids + index_in_node_, sizeof(RID));
        index_in_node_++;
        return RC::SUCCESS;
      }
    }

    index_in_node_ = 0;
  }
  return RC::RECORD_NO_MORE_IDX_IN_MEM;
}
bool BplusTreeScanner::satisfy_condition(const char *pkey)
{
  int i1 = 0, i2 = 0;
  float f1 = 0, f2 = 0;
  const char *s1 = nullptr, *s2 = nullptr;

  if (comp_op_ == NO_OP) {
    return true;
  }

  AttrType attr_type = index_handler_.file_header_.attr_type;
  switch (attr_type) {
    case INTS:
      i1 = *(int *)pkey;
      i2 = *(int *)value_;
      break;
    case FLOATS:
      f1 = *(float *)pkey;
      f2 = *(float *)value_;
      break;
    case CHARS:
      s1 = pkey;
      s2 = value_;
      break;
    default:
      LOG_PANIC("Unknown attr type: %d", attr_type);
  }

  bool flag = false;

  int attr_length = index_handler_.file_header_.attr_length;
  switch (comp_op_) {
    case EQUAL_TO:
      switch (attr_type) {
        case INTS:
          flag = (i1 == i2);
          break;
        case FLOATS:
          flag = 0 == float_compare(f1, f2);
          break;
        case CHARS:
          flag = (strncmp(s1, s2, attr_length) == 0);
          break;
        default:
          LOG_PANIC("Unknown attr type: %d", attr_type);
      }
      break;
    case LESS_THAN:
      switch (attr_type) {
        case INTS:
          flag = (i1 < i2);
          break;
        case FLOATS:
          flag = (f1 < f2);
          break;
        case CHARS:
          flag = (strncmp(s1, s2, attr_length) < 0);
          break;
        default:
          LOG_PANIC("Unknown attr type: %d", attr_type);
      }
      break;
    case GREAT_THAN:
      switch (attr_type) {
        case INTS:
          flag = (i1 > i2);
          break;
        case FLOATS:
          flag = (f1 > f2);
          break;
        case CHARS:
          flag = (strncmp(s1, s2, attr_length) > 0);
          break;
        default:
          LOG_PANIC("Unknown attr type: %d", attr_type);
      }
      break;
    case LESS_EQUAL:
      switch (attr_type) {
        case INTS:
          flag = (i1 <= i2);
          break;
        case FLOATS:
          flag = (f1 <= f2);
          break;
        case CHARS:
          flag = (strncmp(s1, s2, attr_length) <= 0);
          break;
        default:
          LOG_PANIC("Unknown attr type: %d", attr_type);
      }
      break;
    case GREAT_EQUAL:
      switch (attr_type) {
        case INTS:
          flag = (i1 >= i2);
          break;
        case FLOATS:
          flag = (f1 >= f2);
          break;
        case CHARS:
          flag = (strncmp(s1, s2, attr_length) >= 0);
          break;
        default:
          LOG_PANIC("Unknown attr type: %d", attr_type);
      }
      break;
    case NOT_EQUAL:
      switch (attr_type) {
        case INTS:
          flag = (i1 != i2);
          break;
        case FLOATS:
          flag = 0 != float_compare(f1, f2);
          break;
        case CHARS:
          flag = (strncmp(s1, s2, attr_length) != 0);
          break;
        default:
          LOG_PANIC("Unknown attr type: %d", attr_type);
      }
      break;
    default:
      LOG_PANIC("Unknown comp op: %d", comp_op_);
  }
  return flag;
}
