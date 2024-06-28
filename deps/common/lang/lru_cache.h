/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022.09.02
//

#pragma once

#include "common/lang/functional.h"
#include "common/lang/unordered_set.h"

namespace common {

template <typename Key, typename Value, typename Hash = hash<Key>, typename Pred = equal_to<Key>>
class LruCache
{

  class ListNode
  {
  public:
    Key   key_;
    Value value_;

    ListNode *prev_ = nullptr;
    ListNode *next_ = nullptr;

  public:
    ListNode(const Key &key, const Value &value) : key_(key), value_(value) {}
  };

  class PListNodeHasher
  {
  public:
    size_t operator()(ListNode *node) const
    {
      if (node == nullptr) {
        return 0;
      }
      return hasher_(node->key_);
    }

  private:
    Hash hasher_;
  };

  class PListNodePredicator
  {
  public:
    bool operator()(ListNode *const node1, ListNode *const node2) const
    {
      if (node1 == node2) {
        return true;
      }

      if (node1 == nullptr || node2 == nullptr) {
        return false;
      }

      return pred_(node1->key_, node2->key_);
    }

  private:
    Pred pred_;
  };

public:
  LruCache(size_t reserve = 0)
  {
    if (reserve > 0) {
      searcher_.reserve(reserve);
    }
  }

  ~LruCache() { destroy(); }

  void destroy()
  {
    for (ListNode *node : searcher_) {
      delete node;
    }
    searcher_.clear();

    lru_front_ = nullptr;
    lru_tail_  = nullptr;
  }

  size_t count() const { return searcher_.size(); }

  bool get(const Key &key, Value &value)
  {
    auto iter = searcher_.find((ListNode *)&key);
    if (iter == searcher_.end()) {
      return false;
    }

    lru_touch(*iter);
    value = (*iter)->value_;
    return true;
  }

  void put(const Key &key, const Value &value)
  {
    auto iter = searcher_.find((ListNode *)&key);
    if (iter != searcher_.end()) {
      ListNode *ln = *iter;
      ln->value_   = value;
      lru_touch(ln);
      return;
    }

    ListNode *ln = new ListNode(key, value);
    lru_push(ln);
  }

  void remove(const Key &key)
  {
    auto iter = searcher_.find((ListNode *)&key);
    if (iter != searcher_.end()) {
      lru_remove(*iter);
    }
  }

  void pop(Value *&value)
  {
    // TODO
    value = nullptr;
  }

  void foreach (function<bool(const Key &, const Value &)> func)
  {
    for (ListNode *node = lru_front_; node != nullptr; node = node->next_) {
      bool ret = func(node->key_, node->value_);
      if (!ret) {
        break;
      }
    }
  }

  void foreach_reverse(function<bool(const Key &, const Value &)> func)
  {
    for (ListNode *node = lru_tail_; node != nullptr; node = node->prev_) {
      bool ret = func(node->key_, node->value_);
      if (!ret) {
        break;
      }
    }
  }

private:
  void lru_touch(ListNode *node)
  {
    // move node to front
    if (nullptr == node->prev_) {
      return;
    }

    node->prev_->next_ = node->next_;

    if (node->next_ != nullptr) {
      node->next_->prev_ = node->prev_;
    } else {
      lru_tail_ = node->prev_;
    }

    node->prev_ = nullptr;
    node->next_ = lru_front_;
    if (lru_front_ != nullptr) {
      lru_front_->prev_ = node;
    }
    lru_front_ = node;
  }

  void lru_push(ListNode *node)
  {
    // push front
    if (nullptr == lru_tail_) {
      lru_tail_ = node;
    }

    node->prev_ = nullptr;
    node->next_ = lru_front_;
    if (lru_front_ != nullptr) {
      lru_front_->prev_ = node;
    }

    lru_front_ = node;
    searcher_.insert(node);
  }

  void lru_remove(ListNode *node)
  {
    if (node->prev_ != nullptr) {
      node->prev_->next_ = node->next_;
    }

    if (node->next_ != nullptr) {
      node->next_->prev_ = node->prev_;
    }

    if (lru_front_ == node) {
      lru_front_ = node->next_;
    }
    if (lru_tail_ == node) {
      lru_tail_ = node->prev_;
    }

    searcher_.erase(node);
    delete node;
  }

private:
  using SearchType = unordered_set<ListNode *, PListNodeHasher, PListNodePredicator>;
  SearchType searcher_;
  ListNode  *lru_front_ = nullptr;
  ListNode  *lru_tail_  = nullptr;
};

}  // namespace common
