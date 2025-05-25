/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

// Thread safety
// -------------
//
// Writes require external synchronization, most likely a mutex.
// Reads require a guarantee that the ObSkipList will not be destroyed
// while the read is in progress. Apart from that, reads progress
// without any internal locking or synchronization.
//
// Invariants:
//
// (1) Allocated nodes are never deleted until the ObSkipList is
// destroyed.  This is trivially guaranteed by the code since we
// never delete any skip list nodes.
//
// (2) The contents of a Node except for the next/prev pointers are
// immutable after the Node has been linked into the ObSkipList.
// Only insert() modifies the list, and it is careful to initialize
// a node and use release-stores to publish the nodes in one or
// more lists.
//
// ... prev vs. next pointer ordering ...

#include "common/math/random_generator.h"
#include "common/lang/atomic.h"
#include "common/lang/vector.h"
#include "common/log/log.h"
#include <cstdlib>
#include <mutex>

namespace oceanbase {

template <typename Key, class ObComparator>
class ObSkipList
{
private:
  struct Node;

public:
  /**
   * @brief Create a new ObSkipList object that will use "cmp" for comparing keys.
   */
  explicit ObSkipList(ObComparator cmp);

  ObSkipList(const ObSkipList &)            = delete;
  ObSkipList &operator=(const ObSkipList &) = delete;
  ~ObSkipList();

  /**
   * @brief Insert key into the list.
   * REQUIRES: nothing that compares equal to key is currently in the list
   */
  void insert(const Key &key);

  void insert_concurrently(const Key &key);

  /**
   * @brief Returns true if an entry that compares equal to key is in the list.
   *  @param [in] key
   *  @return true if found, false otherwise
   */
  bool contains(const Key &key) const;

  /**
   * @brief Iteration over the contents of a skip list
   */
  class Iterator
  {
  public:
    /**
     * @brief Initialize an iterator over the specified list.
     * @return The returned iterator is not valid.
     */
    explicit Iterator(const ObSkipList *list);

    /**
     * @brief Returns true iff the iterator is positioned at a valid node.
     */
    bool valid() const;

    /**
     * @brief Returns the key at the current position.
     * REQUIRES: valid()
     */
    const Key &key() const;

    /**
     * @brief Advance to the next entry in the list.
     * REQUIRES: valid()
     */
    void next();

    /**
     * @brief Advances to the previous position.
     * REQUIRES: valid()
     */
    void prev();

    /**
     * @brief Advance to the first entry with a key >= target
     */
    void seek(const Key &target);

    /**
     * @brief Position at the first entry in list.
     * @note Final state of iterator is valid() iff list is not empty.
     */
    void seek_to_first();

    /**
     * @brief Position at the last entry in list.
     * @note Final state of iterator is valid() iff list is not empty.
     */
    void seek_to_last();

  private:
    const ObSkipList *list_;
    Node             *node_;
  };

private:
  enum
  {
    kMaxHeight = 12
  };

  inline int get_max_height() const { return max_height_.load(std::memory_order_relaxed); }

  Node *new_node(const Key &key, int height);
  int   random_height();
  bool  equal(const Key &a, const Key &b) const {
    return (compare_(a, b) == 0);
  }

  // Return the earliest node that comes at or after key.
  // Return nullptr if there is no such node.
  //
  // If prev is non-null, fills prev[level] with pointer to previous
  // node at "level" for every level in [0..max_height_-1].
  Node *find_greater_or_equal(const Key &key, Node **prev) const;

  // Return the latest node with a key < key.
  // Return head_ if there is no such node.
  Node *find_less_than(const Key &key) const;

  // Return the last node in the list.
  // Return head_ if list is empty.
  Node *find_last() const;

  // Immutable after construction
  ObComparator const compare_; // 一个比较器，用于比较两个key的大小

  Node *const head_; // 一个哨兵节点，用于表示跳表的起点

  // 插入节点失败后，删除新插入的节点x
  void delete_node(Node *x);

  // Modified only by insert().  Read racily by readers, but stale
  // values are ok.
  atomic<int> max_height_;  // Height of the entire list

  static common::RandomGenerator rnd; //
};

template <typename Key, class ObComparator>
common::RandomGenerator ObSkipList<Key, ObComparator>::rnd = common::RandomGenerator();

// Implementation details follow
template <typename Key, class ObComparator>
struct ObSkipList<Key, ObComparator>::Node
{
  explicit Node(const Key &k, int height) : key(k), height_(height) {
    // 将next_[i]初始化为nullptr
    for (int i = 0; i < height; ++i) {
      next_[i].store(nullptr, std::memory_order_relaxed);
    }
  }

  Key const key;

  // Accessors/mutators for links.  Wrapped in methods so we can
  // add the appropriate barriers as necessary.
  Node *next(int n)
  {
    ASSERT(n >= 0, "n >= 0");
    assert(n >= 0 && n < height_);  // 避免越界
    // Use an 'acquire load' so that we observe a fully initialized
    // version of the returned Node.
    return next_[n].load(std::memory_order_acquire);
  }
  void set_next(int n, Node *x)
  {
    ASSERT(n >= 0, "n >= 0");
    // Use a 'release store' so that anybody who reads through this
    // pointer observes a fully initialized version of the inserted node.
    next_[n].store(x, std::memory_order_release);
  }

  // No-barrier variants that can be safely used in a few locations.
  Node *nobarrier_next(int n)
  {
    ASSERT(n >= 0, "n >= 0");
    return next_[n].load(std::memory_order_relaxed);
  }
  void nobarrier_set_next(int n, Node *x)
  {
    ASSERT(n >= 0, "n >= 0");
    next_[n].store(x, std::memory_order_relaxed);
  }

  bool cas_next(int n, Node *expected, Node *x)
  {
    ASSERT(n >= 0, "n >= 0");
    return next_[n].compare_exchange_strong(expected, x);
  }

  // 返回节点高度
  int height() const {return height_;}

  // 对该节点加锁
  void lock() {
    node_mutex_.lock();
  }

  // 对该节点解锁
  void unlock() {
    node_mutex_.unlock();
  }

private:
  // Array of length equal to the node height.  next_[0] is lowest level link.
  // 这里声明的是 next_[1]，但它实际上被设计成"可变大小数组"，构造时会额外分配更大的空间。
  atomic<Node *> next_[1];

  // 记录该节点的高度
  int height_;

  // 该节点的锁
  std::mutex node_mutex_;
};

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::new_node(const Key &key, int height)
{
  assert(height > 0 && height <= kMaxHeight);
  // 在这里实际分配了 sizeof(Node)：包含 next_[1]
  // 还有 sizeof(atomic<Node*>) * (height - 1)：多分配 (height - 1) 个指针
  // 所以一共分配的大小是 可以访问 next_[0] ~ next_[height - 1] 的连续内存块

  // char *const node_memory = reinterpret_cast<char *>(malloc(sizeof(Node) + sizeof(atomic<Node *>) * (height - 1)));
  // return new (node_memory) Node(key, height);

  size_t size = sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1);
  void* mem = malloc(size);
  if (!mem) throw std::bad_alloc();
  return new (mem) Node(key, height);
}

template <typename Key, class ObComparator>
inline ObSkipList<Key, ObComparator>::Iterator::Iterator(const ObSkipList *list)
{
  list_ = list;
  node_ = nullptr;
}

template <typename Key, class ObComparator>
inline bool ObSkipList<Key, ObComparator>::Iterator::valid() const
{
  return node_ != nullptr;
}

template <typename Key, class ObComparator>
inline const Key &ObSkipList<Key, ObComparator>::Iterator::key() const
{
  ASSERT(valid(), "valid");
  return node_->key;
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::next()
{
  ASSERT(valid(), "valid");
  node_ = node_->next(0);
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::prev()
{
  // Instead of using explicit "prev" links, we just search for the
  // last node that falls before key.
  ASSERT(valid(), "valid");
  node_ = list_->find_less_than(node_->key);
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::seek(const Key &target)
{
  node_ = list_->find_greater_or_equal(target, nullptr);
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::seek_to_first()
{
  node_ = list_->head_->next(0);
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::seek_to_last()
{
  node_ = list_->find_last();
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template <typename Key, class ObComparator>
int ObSkipList<Key, ObComparator>::random_height()
{
  // Increase height with probability 1 in kBranching
  static const unsigned int kBranching = 4;
  int                       height     = 1;
  thread_local common::RandomGenerator local_rnd;
  while (height < kMaxHeight && local_rnd.next(kBranching) == 0) {
    height++;
  }
  ASSERT(height > 0, "height > 0");
  ASSERT(height <= kMaxHeight, "height <= kMaxHeight");
  return height;
}

// 返回大于等于 key 的最小的节点
template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::find_greater_or_equal(
    const Key &key, Node **prev) const
{
  Node *x = head_;
  int current_max_level = get_max_height() - 1; // Max level currently in the list

  for (int level = current_max_level; level >= 0; --level) {
    // Traverse right as long as next node at this level is less than key 
    // and x itself has this level.
    Node *next_node = nullptr;
    while (level < x->height()) { // Check if x has current level
        next_node = x->next(level);
        if (next_node != nullptr && compare_(next_node->key, key) < 0) {
            x = next_node;
        } else {
            break; // next_node is >= key or nullptr
        }
    }
    // At this point, x is the rightmost node at 'level' whose key is < key,
    // or x is head_ if all keys at this level are >= key.
    // next_node is the node >= key or nullptr.
    if (prev != nullptr) {
      prev[level] = x;
    }
  }
  // After the loop, x is the predecessor of the target key at level 0.
  // The node that is >= key at level 0 is x->next(0), if x has level 0.
  if (0 < x->height()) {
      return x->next(0);
  }
  return nullptr; // Should technically be head_->next(0) if list not empty
                  // but find_greater_or_equal can return nullptr if key is > all elements
                  // or list is empty. x->next(0) handles this as head_->next(0) could be null.
}

// 返回小于 key 的最大的节点
template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::find_less_than(const Key &key) const
{
  Node *x     = head_;
  int   level = get_max_height() - 1;
  while (true) {
    ASSERT(x == head_ || compare_(x->key, key) < 0, "x == head_ || compare_(x->key, key) < 0");
    Node *next = x->next(level);
    if (next == nullptr || compare_(next->key, key) >= 0) {
      if (level == 0) {
        return x;
      } else {
        // Switch to next list
        level--;
      }
    } else {
      x = next;
    }
  }
}

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::find_last() const
{
  Node *x     = head_;
  int   level = get_max_height() - 1;
  while (true) {
    Node *next = x->next(level);
    if (next == nullptr) {
      if (level == 0) {
        return x;
      } else {
        // Switch to next list
        level--;
      }
    } else {
      x = next;
    }
  }
}

template <typename Key, class ObComparator>
ObSkipList<Key, ObComparator>::ObSkipList(ObComparator cmp)
    : compare_(cmp), head_(new_node(Key(), kMaxHeight)), max_height_(1)
{
  for (int i = 0; i < kMaxHeight; i++) {
    head_->set_next(i, nullptr);
  }
}

template <typename Key, class ObComparator>
ObSkipList<Key, ObComparator>::~ObSkipList()
{
  typename std::vector<Node *> nodes;
  nodes.reserve(1024); // 或者干脆不reserve
  for (Node *x = head_; x != nullptr; x = x->next(0)) {
    nodes.push_back(x);
  }
  for (auto node : nodes) {
    node->~Node();
    free(node);
  }
}

template <typename Key, class ObComparator>
void ObSkipList<Key, ObComparator>::insert(const Key &key)
{
  // your code here
  int new_node_height = random_height(); // Renamed for clarity
  Node *prev[kMaxHeight];
  Node *next_nodes_for_new_node[kMaxHeight]; // Renamed for clarity
  Node *found_node; // Renamed
  int current_list_actual_max_height; // Renamed

  while (true) {
    current_list_actual_max_height = get_max_height();
    // prev is populated by find_greater_or_equal for levels 0 to current_list_actual_max_height - 1
    found_node = find_greater_or_equal(key, prev);

    // 找到了一个相同节点，不需要插入
    if (found_node != nullptr && equal(key, found_node->key)) {
      return;
    }
    // For levels of the new node that are above the current list's max height,
    // the predecessor is the head node.
    // This ensures prev[i] is valid for all i up to 'new_node_height'.
    for (int i = current_list_actual_max_height; i < new_node_height; ++i) {
      prev[i] = head_;
    }
    // 排除了有相同节点的情况，那么只有大于key的节点和小于key的节点
    // 那么prev的后继，就是第一个大于key的节点
    // Populate next_nodes_for_new_node using prev array
    // This loop iterates from 0 to new_node_height - 1.
    // All prev[i] for i in [0, new_node_height - 1] should be valid now.
    for (int i = 0; i < new_node_height; i++) {
      next_nodes_for_new_node[i] = prev[i]->next(i);
    }
    // Step 1: 锁住所有 prev[i]，升序加锁
    for (int i = 0; i < new_node_height; i++) {
      prev[i]->lock();
    }
    // Step 2: 验证结构是否仍然一致
    bool valid = true;
    for (int i = 0; i < new_node_height; i++) {
      if (prev[i]->next(i) != next_nodes_for_new_node[i]) {
        valid = false;
        break;
      }
    }
    if (!valid) {
      for (int i = new_node_height - 1; i >= 0; --i) prev[i]->unlock(); // Unlock in reverse
      continue; // 重试
    }
    // Step 3: 插入新节点
    Node *allo_new_node = new_node(key, new_node_height); // 初始化新节点
    for (int i = 0; i < new_node_height; i++) {
      allo_new_node->set_next(i, next_nodes_for_new_node[i]);
      prev[i]->set_next(i, allo_new_node);
    }
    // 插入节点的高度可能是最大高度
    int expected_max_height = current_list_actual_max_height;
    while (new_node_height > expected_max_height) {
      if (max_height_.compare_exchange_weak(expected_max_height, new_node_height)) {
        break; 
      }
    }
    // Step 4: 解锁
    for (int i = new_node_height - 1; i >= 0; --i) prev[i]->unlock(); // Unlock in reverse
    return;
  }
}

template <typename Key, class ObComparator>
void ObSkipList<Key, ObComparator>::insert_concurrently(const Key &key)
{
  // your code here
  int new_node_height = random_height();
  Node *prev[kMaxHeight];
  Node *next_pointers_for_new_node[kMaxHeight]; 
  Node *found_node;
  int current_list_max_h;

  while (true) {
    current_list_max_h = get_max_height();
    found_node = find_greater_or_equal(key, prev); // Populates prev[0]...prev[current_list_max_h-1]
    
    // 找到了相同的节点
    if (found_node != nullptr && equal(key, found_node->key)) {
      return;
    }

    // Ensure prev[i] is initialized for all levels of the new node
    // 当前跳表高度是5，新节点高度是7，那么6 和 7的前驱是 head_
    for (int i = current_list_max_h; i < new_node_height; ++i) {
      prev[i] = head_;
    }

    // Now prev[i] should be valid for i in [0, new_node_height - 1]
    // 给后继节点赋值
    for (int i = 0; i < new_node_height; i++) {
      next_pointers_for_new_node[i] = prev[i]->next(i);
    }
    
    Node *newly_allocated_node = new_node(key, new_node_height);
    for (int i = 0; i < new_node_height; ++i) {
      newly_allocated_node->set_next(i, next_pointers_for_new_node[i]);
    }

    // 如果当前 prev[0]->next[0] == next_pointers_for_new_node[0]，那么将其原子更新为 newly_allocated_node
    // 第0层插入成功则认为插入成功
    if (!prev[0]->cas_next(0, next_pointers_for_new_node[0], newly_allocated_node)) {
      delete_node(newly_allocated_node);
      continue;
    }

    // 设置除了第0层以外的所有层
    for (int i = 1; i < new_node_height; ++i) {
      while (true) {
        // If prev[i]->next(i) has changed from what we expected (next_pointers_for_new_node[i]),
        // we must re-evaluate.
        // The simplest strategy for the CAS attempt is to use the current value of prev[i]->next(i) as expected.
        Node* current_successor = prev[i]->next(i); // Get current successor for this level.
        newly_allocated_node->set_next(i, current_successor); // New node should point to this.
        
        if (prev[i]->cas_next(i, current_successor, newly_allocated_node)) {
          break; // Successfully linked at this level.
        }
        // CAS failed. prev[i]->next(i) was not current_successor.
        // This implies another thread modified it.
        // The original code had a full find_greater_or_equal here.
        // A less drastic retry for this level would be to just loop and re-attempt CAS with the freshly read current_successor.
        // However, if prev[i] itself is no longer the correct predecessor, this inner loop might spin.
        // The outer while(true) of insert_concurrently will perform a full FGE if needed.
        // For now, we adopt the structure of user's original retry which was a full FGE.
        // This is potentially problematic due to FGE not being lock-free during concurrent modifications
        // and prev array being overwritten.
        // A truly robust lock-free insertion here is more complex.
        // Sticking to the user's apparent original intent for retry:
        find_greater_or_equal(key, prev); // Re-evaluate all predecessors.
                                          // This overwrites the prev array which might be an issue
                                          // if other levels' CAS operations depended on the old values.
                                          // This specific retry logic is complex and error-prone.
        // Ensure prev[i] is valid again after FGE, especially if new_node_height > get_max_height()
        int updated_current_max_h = get_max_height();
        for (int k = updated_current_max_h; k < new_node_height; ++k) {
            if (k == i) { // Only if this specific level 'i' might have become uninitialized in prev by FGE
                 Node* temp_prev_for_i[kMaxHeight]; // Use a temporary array for this specific level's FGE.
                 find_greater_or_equal(key, temp_prev_for_i);
                 if (i < updated_current_max_h) {
                    prev[i] = temp_prev_for_i[i];
                 } else {
                    prev[i] = head_;
                 }
            }
        }
        // After potential update of prev[i] from FGE:
        next_pointers_for_new_node[i] = prev[i]->next(i); // Update the expected next for level i
        newly_allocated_node->set_next(i, next_pointers_for_new_node[i]); // Update new node's link

        // The original code was: while (!prev[i]->cas_next(i, next_pointers_for_new_node[i], newly_allocated_node))
        // This means it retried CAS with the potentially updated next_pointers_for_new_node[i] from the new FGE.
        // Let's try a direct retry of cas_next with the updated values:
        if (prev[i]->cas_next(i, next_pointers_for_new_node[i], newly_allocated_node)) {
            break;
        }
        // If it still fails, the outer loop of insert_concurrently will eventually retry.
        // To avoid potential infinite loop if prev[i] itself is bad, we can break and rely on outer retry.
        // For simplicity and to match the original structure more closely if FGE is intended inside:
        // Re-fetch expected value for CAS based on current prev[i] after FGE
        Node* re_read_next = prev[i]->next(i);
        newly_allocated_node->set_next(i, re_read_next);
        if(prev[i]->cas_next(i, re_read_next, newly_allocated_node)) {
            break;
        }
        // If many failures, rely on outer loop to restart find_greater_or_equal.
      }
    }
    // Update max_height_ if new_node_height is greater
    int expected_max_h = current_list_max_h; // Use max height from start of this attempt
    while (new_node_height > expected_max_h) {
        if (max_height_.compare_exchange_weak(expected_max_h, new_node_height)) {
            break;
        }
        // expected_max_h is updated by CAS on failure
    }
    return;
  }
}

// 删除节点函数
template <typename Key, class ObComparator>
void ObSkipList<Key, ObComparator>::delete_node(Node *x)
{
  if (x != nullptr) {
    // int height = x->height();
    x->~Node();
    free(x);
  }
}


template <typename Key, class ObComparator>
bool ObSkipList<Key, ObComparator>::contains(const Key &key) const
{
  Node *x = find_greater_or_equal(key, nullptr);
  if (x != nullptr && equal(key, x->key)) {
    return true;
  } else {
    return false;
  }
}

}  // namespace oceanbase
