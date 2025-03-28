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
  bool  equal(const Key &a, const Key &b) const { return (compare_(a, b) == 0); }

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
  ObComparator const compare_;

  Node *const head_;

  // Modified only by insert().  Read racily by readers, but stale
  // values are ok.
  atomic<int> max_height_;  // Height of the entire list

  static common::RandomGenerator rnd;
};

template <typename Key, class ObComparator>
common::RandomGenerator ObSkipList<Key, ObComparator>::rnd = common::RandomGenerator();

// Implementation details follow
template <typename Key, class ObComparator>
struct ObSkipList<Key, ObComparator>::Node
{
  explicit Node(const Key &k) : key(k) {}

  Key const key;

  // Accessors/mutators for links.  Wrapped in methods so we can
  // add the appropriate barriers as necessary.
  Node *next(int n)
  {
    ASSERT(n >= 0, "n >= 0");
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

private:
  // Array of length equal to the node height.  next_[0] is lowest level link.
  atomic<Node *> next_[1];
};

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::new_node(const Key &key, int height)
{
  char *const node_memory = reinterpret_cast<char *>(malloc(sizeof(Node) + sizeof(atomic<Node *>) * (height - 1)));
  return new (node_memory) Node(key);
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
  while (height < kMaxHeight && rnd.next(kBranching) == 0) {
    height++;
  }
  ASSERT(height > 0, "height > 0");
  ASSERT(height <= kMaxHeight, "height <= kMaxHeight");
  return height;
}

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::find_greater_or_equal(
    const Key &key, Node **prev) const
{
  // your code here
  return nullptr;
}

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
    : compare_(cmp), head_(new_node(0 /* any key will do */, kMaxHeight)), max_height_(1)
{
  for (int i = 0; i < kMaxHeight; i++) {
    head_->set_next(i, nullptr);
  }
}

template <typename Key, class ObComparator>
ObSkipList<Key, ObComparator>::~ObSkipList()
{
  typename std::vector<Node *> nodes;
  nodes.reserve(max_height_.load(std::memory_order_relaxed));
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
{}

template <typename Key, class ObComparator>
void ObSkipList<Key, ObComparator>::insert_concurrently(const Key &key)
{
  // your code here
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
