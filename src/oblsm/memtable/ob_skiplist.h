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
// Lock-free operations (insert_concurrently, find_lock_free) are designed for concurrent access.
// Other operations like iterator or original find_greater_or_equal may not be fully concurrent-safe
// without further adaptation.
//
// Invariants:
//
// (1) (For lock-free) Nodes are logically deleted by marking edges. Physical reclamation is a separate concern.
//
// (2) The contents of a Node except for the next/prev pointers are
// immutable after the Node has been linked into the ObSkipList.
// Only insert_concurrently() modifies the list structure.
//

#include "common/math/random_generator.h"
#include "common/lang/atomic.h"
#include "common/lang/vector.h"
#include "common/log/log.h"
#include <cstdlib>
#include <mutex> // Still used by Node in original code, but not directly by insert_concurrently
#include <unordered_set> // Was used by old insert
#include <utility> // For std::pair and std::tie

namespace oceanbase {

template <typename Key, class ObComparator>
class ObSkipList
{
private:
  struct Node; // Forward declaration

  // Helper functions for marked pointers (uintptr_t based)
  // Mark is stored in the LSB of the uintptr_t
  static inline bool get_mark_from_uintptr(uintptr_t val) {
      return val & 1;
  }

  static inline Node* get_ptr_from_uintptr(uintptr_t val) {
      // Mask out the LSB (mark bit)
      return reinterpret_cast<Node*>(val & ~static_cast<uintptr_t>(1));
  }

  static inline uintptr_t make_marked_uintptr(Node* p, bool mark) {
      return reinterpret_cast<uintptr_t>(p) | (mark ? 1 : 0);
  }

  struct Node
  {
    explicit Node(const Key &k, int h) : key(k), height_(h) {
        // Initialize next_ array with null pointer, mark false
        for (int i = 0; i < height_; ++i) {
          next_[i].store(make_marked_uintptr(nullptr, false), std::memory_order_relaxed);
        }
    }

    Key const key;
    int height_;

    // Accessors/mutators for links.
    
    // Gets the pointer and mark for a given level
    std::pair<Node*, bool> get_next(int level)
    {
        // LOG_INFO("Node::get_next: this=%p, level=%d, current_node_height=%d", static_cast<void*>(this), level, height_);
        printf("Node::get_next: this=%p, level=%d, current_node_height=%d\n", static_cast<void*>(this), level, height_);
        ASSERT(level >= 0 && level < height_, "level out of bounds in get_next");
        uintptr_t val = next_[level].load(std::memory_order_acquire);
        return {get_ptr_from_uintptr(val), get_mark_from_uintptr(val)};
    }
    
    // Atomically sets the next pointer and mark for a given level.
    // Primarily for initialization or when exclusive access is guaranteed.
    void set_next_atomic(int level, Node* node_ptr, bool mark)
    {
        ASSERT(level >= 0 && level < height_, "level out of bounds in set_next_atomic");
        next_[level].store(make_marked_uintptr(node_ptr, mark), std::memory_order_release);
    }
    
    // CAS operation for next pointer and its mark
    bool cas_next(int level, Node* expected_ptr, bool expected_mark, Node* new_ptr, bool new_mark)
    {
        ASSERT(level >= 0 && level < height_, "level out of bounds in cas_next");
        uintptr_t expected_val = make_marked_uintptr(expected_ptr, expected_mark);
        uintptr_t new_val = make_marked_uintptr(new_ptr, new_mark);
        return next_[level].compare_exchange_strong(expected_val, new_val);
    }

    // Returns node height
    int height() const {return height_;}

    // Mutex for the original insert method, not directly used by new insert_concurrently path
    // This mutex is now unused if the old insert method is removed.
    // std::mutex node_mutex_; 
    // void lock() { node_mutex_.lock(); }
    // void unlock() { node_mutex_.unlock(); }

  private:
    // Array of length equal to the node height.  next_[0] is lowest level link.
    std::atomic<uintptr_t> next_[1]; 
  };

public:
  /**
   * @brief Create a new ObSkipList object that will use "cmp" for comparing keys.
   */
  explicit ObSkipList(ObComparator cmp);

  ObSkipList(const ObSkipList &)            = delete;
  ObSkipList &operator=(const ObSkipList &) = delete;
  ~ObSkipList();

  void insert(const Key &key); // ADDED BACK - locked insert

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

  Node *find_greater_or_equal(const Key &key, Node **prev) const;
  Node *find_less_than(const Key &key) const;
  Node *find_last() const;

  bool find_lock_free(const Key& key_to_find, Node** preds, Node** succs);

  ObComparator const compare_;
  Node *const head_;
  std::mutex list_mutex_; // Mutex for the locked insert operation

  // Renamed from delete_node to clarify its specific use-case for lock-free insertion path
  void cleanup_unlinked_new_node(Node *x) {
    if (x != nullptr) {
        x->~Node(); 
        free(x);    
    }
  }

  atomic<int> max_height_;
  static common::RandomGenerator rnd;
};

template <typename Key, class ObComparator>
common::RandomGenerator ObSkipList<Key, ObComparator>::rnd = common::RandomGenerator();

template <typename Key, class ObComparator>
ObSkipList<Key, ObComparator>::ObSkipList(ObComparator cmp)
    : compare_(cmp), head_(new_node(Key(), kMaxHeight)), max_height_(1)
{
}

template <typename Key, class ObComparator>
ObSkipList<Key, ObComparator>::~ObSkipList()
{
  std::vector<Node *> nodes_to_delete;
  Node* curr = nullptr;
  std::tie(curr, std::ignore) = head_->get_next(0); 

  nodes_to_delete.push_back(head_); 

  while(curr != nullptr) {
    nodes_to_delete.push_back(curr);
    Node* temp_next_node = nullptr; // Store next before potentially invalidating curr's next
    std::tie(temp_next_node, std::ignore) = curr->get_next(0);
    // If physical deletion happens and nodes are reused, this simple traversal might be problematic.
    // Assuming for now this destructor is called when no other operations are active.
    curr = temp_next_node;
  }
  
  for (Node* node_ptr : nodes_to_delete) {
    node_ptr->~Node();
    free(node_ptr);
  }
}

template <typename Key, class ObComparator>
void ObSkipList<Key, ObComparator>::insert(const Key &key) {
    std::lock_guard<std::mutex> lock(list_mutex_);

    Node* update_nodes[kMaxHeight]; 
    Node* current_search_node = head_;
    int current_list_height = get_max_height(); 

    for (int i = current_list_height - 1; i >= 0; --i) {
        Node* next_ptr;
        std::tie(next_ptr, std::ignore) = current_search_node->get_next(i);
        while (next_ptr != nullptr && compare_(next_ptr->key, key) < 0) {
            current_search_node = next_ptr;
            std::tie(next_ptr, std::ignore) = current_search_node->get_next(i);
        }
        update_nodes[i] = current_search_node;
    }

    Node* existing_candidate_node;
    std::tie(existing_candidate_node, std::ignore) = update_nodes[0]->get_next(0);

    if (existing_candidate_node != nullptr && compare_(existing_candidate_node->key, key) == 0) {
        bool is_logically_deleted;
        // Check the mark on the edge FROM existing_candidate_node TO its L0 successor.
        // If this edge is marked, existing_candidate_node itself is considered logically deleted.
        std::tie(std::ignore, is_logically_deleted) = existing_candidate_node->get_next(0); 
        if (!is_logically_deleted) {
            return; // Key truly exists and is not marked as deleted.
        }
        // If it IS logically deleted, we proceed to insert a new node for this key.
    }

    int new_node_actual_height = random_height();
    Node* node_to_insert = new_node(key, new_node_actual_height);

    if (new_node_actual_height > current_list_height) {
        for (int i = current_list_height; i < new_node_actual_height; ++i) {
            update_nodes[i] = head_; 
        }
        
        int expected_h = current_list_height;
        // Atomically update max_height_ using CAS loop
        while (new_node_actual_height > expected_h) {
            if (max_height_.compare_exchange_weak(expected_h, new_node_actual_height)) {
                break;
            }
            // expected_h is updated by CAS on failure, retry.
        }
    }

    for (int i = 0; i < new_node_actual_height; ++i) {
        Node* old_successor;
        std::tie(old_successor, std::ignore) = update_nodes[i]->get_next(i);
        node_to_insert->set_next_atomic(i, old_successor, false);
        update_nodes[i]->set_next_atomic(i, node_to_insert, false);
    }
}

template <typename Key, class ObComparator>
void ObSkipList<Key, ObComparator>::insert_concurrently(const Key &key)
{
  int new_node_height = random_height();
  Node* preds[kMaxHeight];
  Node* succs[kMaxHeight];
  Node* node_to_insert = nullptr; // Allocate once and reuse if CAS fails but node is still valid

  while (true) {
    bool key_exists = find_lock_free(key, preds, succs);
    if (key_exists) {
      if (node_to_insert != nullptr && node_to_insert->height() != new_node_height) {
        // Key was inserted by another thread while we were trying. Clean up our allocated node.
        cleanup_unlinked_new_node(node_to_insert);
        node_to_insert = nullptr;
      }
      return; // Key already exists
    }

    if (node_to_insert == nullptr) { // First attempt or previous node_to_insert was cleaned up.
        node_to_insert = new_node(key, new_node_height);
    } else {
        // node_to_insert was allocated in a previous failed attempt, reuse it.
        // Ensure its height matches the current random_height for this attempt.
        // This is tricky: if random_height changed, the existing node_to_insert might be wrong size.
        // Simplest for now: if new_node_height has changed between retries, reallocate.
        // However, random_height is called ONCE outside the loop. So, new_node_height is fixed per call.
        // So, this 'else' branch implies we are retrying with the SAME node_to_insert.
    }


    // Set next pointers for the new node based on current successors from find_lock_free
    for (int i = 0; i < new_node_height; ++i) {
      node_to_insert->set_next_atomic(i, succs[i], false); 
    }

    if (!preds[0]->cas_next(0, succs[0], false, node_to_insert, false)) {
      // CAS failed at level 0, retry the whole operation from find_lock_free.
      // node_to_insert is kept for the next attempt. Its next pointers will be reset.
      continue; 
    }

    for (int level = 1; level < new_node_height; ++level) {
      while (true) {
        if (level >= node_to_insert->height()) {
          break; // Safety guard: never access next_[level] if beyond height
        }
        if (preds[level]->cas_next(level, succs[level], false, node_to_insert, false)) {
          break; 
        }
        // CAS failed at this higher level. Must re-evaluate.
        // This re-find could potentially help other threads by performing cleanup.
        find_lock_free(key, preds, succs); 

        // After re-finding, if node_to_insert is already correctly linked at this level
        // by another thread or by find_lock_free's cleanup, then we can break.
        Node* current_pred_next_ptr;
        bool current_pred_next_mark;
        std::tie(current_pred_next_ptr, current_pred_next_mark) = preds[level]->get_next(level);

        if (current_pred_next_ptr == node_to_insert && !current_pred_next_mark) {
          break; 
        }
        
        // If succs[level] changed, update node_to_insert's forward pointer for this level before retrying CAS
        // This ensures node_to_insert correctly points to the new successor.
        if (level < node_to_insert->height()) {
          node_to_insert->set_next_atomic(level, succs[level], false);
        }
        
        // Also check if the key is now "fully present" due to another thread's action.
        // If succs[0] (the node at level 0 that would be our key) is now NOT our node_to_insert,
        // it implies another thread completed an insert of the same key.
        // In such a case, our current insertion attempt for node_to_insert has become invalid.
        if (succs[0] != node_to_insert && succs[0] != nullptr && equal(succs[0]->key, key)) {
            // Another thread inserted the same key. Our `node_to_insert` is now redundant.
            // We've already linked at level 0. This is a complex state.
            // Ideally, the top-level `find_lock_free` should catch this in the next major iteration.
            // For this inner loop, the main goal is to correctly link `preds[level]` to `node_to_insert`.
            // If `preds[level]`'s successor has changed such that `node_to_insert` is no longer
            // the correct node to insert after `preds[level]`, this loop will continue.
            // The `find_lock_free` at the start of the outer `while(true)` is the main guard against duplicate keys.
        }
      }
    }
    
    int current_max_h = get_max_height();
    while (new_node_height > current_max_h) {
      if (max_height_.compare_exchange_weak(current_max_h, new_node_height)) {
        break; 
      }
    }
    
    return; 
  }
}

template <typename Key, class ObComparator>
bool ObSkipList<Key, ObComparator>::contains(const Key &key) const
{
  Node* preds[kMaxHeight]; 
  Node* succs[kMaxHeight];
  // find_lock_free can modify the list (physical deletion of marked nodes).
  // A truly const contains method would need a find variant that doesn't do physical deletion.
  // Casting away const is generally undesirable but a common workaround in such retrofits.
  // bool found = const_cast<ObSkipList*>(this)->find_lock_free(key, preds, succs);
  // return found;

  // More accurate (but still not perfect without a non-modifying find):
  // Check if succs[0] is the key and is not marked.
  // The find_lock_free returns true if succs[0] is the key and its link from preds[0] is not marked,
  // AND its own primary link (level 0 next) is not marked.
  if (const_cast<ObSkipList*>(this)->find_lock_free(key, preds, succs)) {
      // Check if succs[0] (the node found) is still valid and not marked for deletion.
      // find_lock_free's return value already implies this.
      return true;
  }
  return false;
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
  ASSERT(valid(), "Iterator not valid");
  return node_->key;
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::next() {
  ASSERT(valid(), "Iterator not valid");
  if (!node_) return;

  Node* current_node = node_;
  while (true) {
    Node* next_node_ptr = nullptr;
    bool edge_is_marked = false;
    std::tie(next_node_ptr, edge_is_marked) = current_node->get_next(0);

    // If the edge to next_node_ptr is marked, current_node is logically deleted.
    // However, the iterator might have landed on current_node when it wasn't yet marked.
    // The iterator should advance to the *next unmarkedly-linked node*.

    if (next_node_ptr == nullptr) { // End of list
        node_ = nullptr;
        return;
    }

    // Check if next_node_ptr itself is logically deleted (by checking its edge to *its* successor)
    Node* nn_succ_ptr = nullptr;
    bool nn_edge_is_marked = false;
    std::tie(nn_succ_ptr, nn_edge_is_marked) = next_node_ptr->get_next(0);

    if (!nn_edge_is_marked) { // If next_node_ptr is not (yet) logically deleted
        node_ = next_node_ptr;
        return;
    }
    // next_node_ptr is logically deleted, so try to advance current_node past it
    current_node = next_node_ptr;
  }
}


template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::prev() {
  ASSERT(valid(), "Iterator not valid");
  // prev() is very hard for lock-free skip lists with physical deletion.
  // The underlying list structure can change dramatically.
  // This simplified version uses the old, non-lock-free find_less_than, which is not safe.
  LOG_WARN("Iterator::prev() is not fully concurrent-safe and may be unreliable.");
  if (!node_) return;
  Node* less_than_node = list_->find_less_than(node_->key); // Uses old find, not mark-aware.
  if (less_than_node == list_->head_) {
    node_ = nullptr; // No node strictly less than current, or current was first.
  } else {
    node_ = less_than_node;
  }
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::seek(const Key &target) {
    // Needs a find that is aware of marked nodes and correctly positions.
    LOG_WARN("Iterator::seek() is not fully concurrent-safe and may be unreliable.");
    // Node* preds[kMaxHeight]; // Removed unused variable
    // Node* succs[kMaxHeight]; // Removed unused variable
    // const_cast<ObSkipList*>(list_)->find_lock_free(target, preds, succs);
    // node_ = succs[0]; // This would be the node >= target
    
    // Fallback to old find for now, it's not mark-aware.
    node_ = list_->find_greater_or_equal(target, nullptr);
    // Iterator should skip over logically deleted nodes if find_greater_or_equal lands on one.
    if (node_) {
        Node* temp_succ_ptr;
        bool is_marked;
        std::tie(temp_succ_ptr, is_marked) = node_->get_next(0);
        if(is_marked) { // If the found node is marked, try to advance.
            next(); // Call the iterator's next() to find the next valid node
        }
    }
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::seek_to_first() {
    LOG_WARN("Iterator::seek_to_first() may not be fully concurrent-safe.");
    Node* curr = list_->head_;
    Node* next_node_ptr = nullptr;
    bool is_marked = false; // Mark of the edge from curr to next_node_ptr
    
    // Traverse from head to find the first non-logically-deleted node
    while(true) {
        std::tie(next_node_ptr, is_marked) = curr->get_next(0); // Get successor of curr
        
        if (next_node_ptr == nullptr) { // Empty list or reached end
            node_ = nullptr;
            return;
        }
        
        // Now, check if next_node_ptr itself is logically deleted.
        // A node is logically deleted if the edge *from it* to *its successor* is marked.
        Node* nn_succ_ptr; // Successor of next_node_ptr
        bool nn_edge_is_marked; // Mark on the edge from next_node_ptr to nn_succ_ptr
        std::tie(nn_succ_ptr, nn_edge_is_marked) = next_node_ptr->get_next(0);

        if (!nn_edge_is_marked) { // Found first non-deleted node (next_node_ptr)
            node_ = next_node_ptr;
            return;
        }
        // next_node_ptr is logically deleted, so advance curr to skip it in the next iteration.
        curr = next_node_ptr; 
    }
}

template <typename Key, class ObComparator>
inline void ObSkipList<Key, ObComparator>::Iterator::seek_to_last() {
    LOG_WARN("Iterator::seek_to_last() is not fully concurrent-safe and is complex.");
    // This is very hard in a lock-free list with physical deletion as "last" can change.
    // A simple traversal using find_last (old version) is not safe.
    // For now, making it a no-op or setting to invalid.
    node_ = nullptr; // Placeholder
}


template <typename Key, class ObComparator>
bool ObSkipList<Key, ObComparator>::find_lock_free(const Key& key_to_find, Node** preds, Node** succs) {
    Node* pred = nullptr;
    Node* curr = nullptr;
    Node* succ = nullptr;
    bool curr_points_to_succ_marked = false; 
    bool snip_successful;

retry_find: 
    pred = head_;
    for (int level = get_max_height() - 1; level >= 0; --level) {
        std::tie(curr, std::ignore) = pred->get_next(level); 

        while (true) {
            if (curr == nullptr) { 
                succ = nullptr;
                curr_points_to_succ_marked = false; 
            } else {
                int curr_height = curr->height(); // avoid multiple calls
                if (level >= curr_height) { // Check if level is valid for curr node
                    succ = nullptr;
                    curr_points_to_succ_marked = false;
                } else {
                    std::tie(succ, curr_points_to_succ_marked) = curr->get_next(level);
                }
            }

            while (curr != nullptr && curr_points_to_succ_marked) {
                snip_successful = pred->cas_next(level, curr, false, succ, false);
                
                if (!snip_successful) {
                    goto retry_find; 
                }
                std::tie(curr, std::ignore) = pred->get_next(level); 

                if (curr == nullptr) { 
                    succ = nullptr;
                    curr_points_to_succ_marked = false;
                } else {
                    if (level >= curr->height()) { // Check if level is valid for curr node
                        succ = nullptr;
                        curr_points_to_succ_marked = false;
                    } else {
                        std::tie(succ, curr_points_to_succ_marked) = curr->get_next(level);
                    }
                }
            }

            if (curr == nullptr || compare_(curr->key, key_to_find) >= 0) {
                break; 
            }
            pred = curr; 
            curr = succ; 
        }
        preds[level] = pred;
        succs[level] = curr;
    }

    if (succs[0] != nullptr && compare_(succs[0]->key, key_to_find) == 0) {
        bool is_succ0_edge_to_its_next_marked;
        std::tie(std::ignore, is_succ0_edge_to_its_next_marked) = succs[0]->get_next(0);
        return !is_succ0_edge_to_its_next_marked; 
    }
    return false;
}

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::new_node(const Key &key, int height)
{
   // LOG_INFO("new_node: creating node with key=%lu, height=%d", static_cast<unsigned long>(key), height);
   printf("new_node: creating node with key_addr=%p, height=%d\n", static_cast<const void*>(&key), height); // Print address of key
    assert(height > 0 && height <= kMaxHeight);
    size_t node_obj_size = sizeof(Node);
    // Calculate size for the flexible array member part (next_)
    // If height is 1, next_[1] is already part of Node, so no *extra* needed for next_ pointers.
    // If height > 1, we need (height - 1) *additional* atomic<uintptr_t>s.
    size_t next_array_extra_size = (height > 1) ? (sizeof(std::atomic<uintptr_t>) * (height - 1)) : 0;
    size_t total_size = node_obj_size + next_array_extra_size;
    
    void* mem = malloc(total_size);
    if (!mem) throw std::bad_alloc();
    return new (mem) Node(key, height);
}

template <typename Key, class ObComparator>
int ObSkipList<Key, ObComparator>::random_height()
{
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

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::find_greater_or_equal(
    const Key &key, Node **prev) const
{
  Node *x = head_;
  int current_max_level = get_max_height() - 1; 

  for (int level = current_max_level; level >= 0; --level) {
    Node *next_node_ptr;
    std::tie(next_node_ptr, std::ignore) = x->get_next(level);

    // This loop needs to be careful: if next_node_ptr is marked, original logic would proceed.
    // For lock-free, marked nodes should typically be skipped or handled by CAS in find_lock_free.
    while (next_node_ptr != nullptr && compare_(next_node_ptr->key, key) < 0) {
        // Before advancing x, ensure next_node_ptr is not logically deleted if this find
        // is to be somewhat safe in a concurrent environment (which it isn't fully).
        // For true safety, this whole method needs a rewrite or to use find_lock_free.
        x = next_node_ptr;
        std::tie(next_node_ptr, std::ignore) = x->get_next(level);
    }
    if (prev != nullptr) {
      prev[level] = x;
    }
  }
  Node* result_node_ptr;
  std::tie(result_node_ptr, std::ignore) = x->get_next(0); // Ignores mark
  // If result_node_ptr is marked, this find returns a logically deleted node.
  return result_node_ptr;
}

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::find_less_than(const Key &key) const {
    Node *x = head_;
    int level = get_max_height() - 1;
    while (true) {
        Node* next_ptr;
        bool is_marked; // Mark of edge from x to next_ptr
        std::tie(next_ptr, is_marked) = x->get_next(level);
        
        // If the path to next_ptr is marked, or next_ptr itself is marked, should skip.
        // This simplified version only considers the mark on the edge from x.
        if (next_ptr == nullptr || is_marked || compare_(next_ptr->key, key) >= 0) {
            if (level == 0) {
                // If x is head_ and all nodes are >= key, this is correct.
                // If x is a normal node, we need to ensure x itself is not marked.
                // This is not fully safe.
                return x;
            } else {
                level--;
            }
        } else {
            x = next_ptr;
        }
    }
}

template <typename Key, class ObComparator>
typename ObSkipList<Key, ObComparator>::Node *ObSkipList<Key, ObComparator>::find_last() const {
    Node *x = head_;
    int level = get_max_height() - 1;
    Node* y = nullptr; // To store the candidate for last node

    // Traverse from highest level
    while(level >= 0) {
        Node* next_ptr;
        bool edge_marked;
        std::tie(next_ptr, edge_marked) = x->get_next(level);

        // While next is valid, unmarked, keep moving right
        while(next_ptr != nullptr && !edge_marked) {
            x = next_ptr;
            std::tie(next_ptr, edge_marked) = x->get_next(level);
        }
        // x is now the rightmost non-logically-deleted-via-edge node at this level.
        // If we are at level 0, x is a candidate for the last node.
        if (level == 0) {
            // Check if x itself is marked (i.e. its edge to its L0 successor is marked)
            bool x_is_logically_deleted;
            std::tie(std::ignore, x_is_logically_deleted) = x->get_next(0);
            if (x != head_ && !x_is_logically_deleted) {
                return x; // x is a valid, non-head, non-deleted last node
            } else if (x == head_ && !x_is_logically_deleted) { // Empty list or all deleted
                 return head_; // Or nullptr if head_ shouldn't be returned
            } else {
                // x is head_ and marked (should not happen) or x is normal node but marked.
                // This implies we need to backtrack or this logic is insufficient.
                // For simplicity, if x is head_ or marked, and we are at L0, it means no valid last node found by this path.
                // This find_last is still not fully robust for lock-free.
                return (x == head_) ? head_ : nullptr; // Simplified
            }
        }
        level--; // Move to next lower level
    }
    return head_; // Should be unreachable if kMaxHeight > 0
}

}  // namespace oceanbase
