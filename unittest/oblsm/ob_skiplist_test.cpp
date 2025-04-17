/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "gtest/gtest.h"

#include "oblsm/memtable/ob_skiplist.h"
#include "common/math/random_generator.h"
#include "common/thread/thread_pool_executor.h"
#include "common/lang/thread.h"

using namespace oceanbase;

using Key = uint64_t;

struct Comparator {
  int operator()(const Key& a, const Key& b) const {
    if (a < b) {
      return -1;
    } else if (a > b) {
      return +1;
    } else {
      return 0;
    }
  }
};

TEST(skiplist_test, DISABLED_skiplist_test_basic)
{
  common::RandomGenerator rnd;
  const int N = 2000;
  const int R = 5000;
  std::set<Key> keys;
  Comparator cmp;
  ObSkipList<Key, Comparator> list(cmp);
  for (int i = 0; i < N; i++) {
    Key key = rnd.next() % R;
    if (keys.insert(key).second) {
      list.insert(key);
    }
  }

  for (int i = 0; i < R; i++) {
    if (list.contains(i)) {
      ASSERT_EQ(keys.count(i), 1);
    } else {
      ASSERT_EQ(keys.count(i), 0);
    }
  }

}

inline uint32_t decode_fixed32(const char* ptr) {
    uint32_t result;
    memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
    return result;
}

uint32_t murmurhash(const char* data, size_t n, uint32_t seed) {
  // https://github.com/aappleby/smhasher/wiki/MurmurHash1
  const uint32_t m = 0xc6a4a793;
  const uint32_t r = 24;
  const char* limit = data + n;
  uint32_t h = static_cast<uint32_t>(seed ^ (n * m));

  while (data + 4 <= limit) {
    uint32_t w = decode_fixed32(data);
    data += 4;
    h += w;
    h *= m;
    h ^= (h >> 16);
  }

  switch (limit - data) {
    case 3:
      h += static_cast<uint32_t>(static_cast<int8_t>(data[2])) << 16;
    case 2:
      h += static_cast<uint32_t>(static_cast<int8_t>(data[1])) << 8;
    case 1:
      h += static_cast<uint32_t>(static_cast<int8_t>(data[0]));
      h *= m;
      h ^= (h >> r);
      break;
  }
  return h;
}

class ConcurrentTest {
 public:
  static const uint32_t K = 8;

 private:
  static uint64_t key(Key key) { return (key >> 40); }
  static uint64_t gen(Key key) { return (key >> 8) & 0xffffffffu; }
  static uint64_t hash(Key key) { return key & 0xff; }

  static uint64_t hash_numbers(uint64_t k, uint64_t g) {
    uint64_t data[2] = {k, g};
    return murmurhash(reinterpret_cast<char*>(data), sizeof(data), 0);
  }

  static Key make_key(uint64_t k, uint64_t g) {
    assert(sizeof(Key) == sizeof(uint64_t));
    assert(k <= K);  // We sometimes pass K to seek to the end of the skiplist
    assert(g <= 0xffffffffu);
    return ((k << 40) | (g << 8) | (hash_numbers(k, g) & 0xff));
  }

  // Per-key generation
  struct State {
    std::atomic<int> generation[K];
    void Set(int k, int v) {
      generation[k].store(v, std::memory_order_release);
    }
    int Get(int k) { return generation[k].load(std::memory_order_acquire); }

    State() {
      for (unsigned int k = 0; k < K; k++) {
        Set(k, 0);
      }
    }
  };

  // Current state of the test
  State current_;

  // InlineSkipList is not protected by mu_.  We just use a single writer
  // thread to modify it.
  ObSkipList<Key, Comparator> list_;

 public:
  ConcurrentTest() : list_(Comparator()) {}
  thread_local static common::RandomGenerator rnd;
  int scan() {
    auto iter = ObSkipList<Key, Comparator>::Iterator(&list_);
    iter.seek_to_first();
    int count = 0;
    while (iter.valid()) {
      count++;
      iter.next();
    }
    return count;
  }
  // REQUIRES: No concurrent calls for the same k
  void concurrent_write_step(uint32_t k) {
    const int g = current_.Get(k) + 1;
    const Key new_key = make_key(k, g);
    list_.insert_concurrently(new_key);
    ASSERT_EQ(g, current_.Get(k) + 1);
    current_.Set(k, g);
  }

};

thread_local common::RandomGenerator ConcurrentTest::rnd = common::RandomGenerator();

const uint32_t ConcurrentTest::K;
using TestInlineSkipList = ObSkipList<Key, Comparator>;
class InlineSkipTest : public testing::Test {
 public:
  void Insert(TestInlineSkipList* list, Key key) {
    list->insert(key);
    keys_.insert(key);
  }

 private:
  std::set<Key> keys_;
};

class TestState {
 public:
  ConcurrentTest t_;
  std::atomic<bool> quit_flag_;
  std::atomic<uint32_t> next_writer_;

  enum ReaderState { STARTING, RUNNING, DONE };

  explicit TestState()
      : quit_flag_(false),
        state_(STARTING),
        pending_writers_(0),
        state_cv_() {}

  void wait(ReaderState s) {
    std::unique_lock<std::mutex> lock(mu_);
    while (state_ != s) {
      state_cv_.wait(lock);
    }
  }

  void change(ReaderState s) {
    std::unique_lock<std::mutex> lock(mu_);
    state_ = s;
    state_cv_.notify_one();
  }

  void adjust_pending_writers(int delta) {
    std::unique_lock<std::mutex> lock(mu_);
    pending_writers_ += delta;
    if (pending_writers_ == 0) {
      state_cv_.notify_one();
    }
  }

  void wait_for_pending_writers() {
    std::unique_lock<std::mutex> lock(mu_);
    while (pending_writers_ != 0) {
      state_cv_.wait(lock);
    }
  }

 private:
  std::mutex mu_;
  ReaderState state_;
  int pending_writers_;
  std::condition_variable state_cv_;
};


static void concurrent_reader(void* arg) {
  TestState* state = static_cast<TestState*>(arg);
  state->change(TestState::RUNNING);
  while (!state->quit_flag_.load(std::memory_order_acquire)) {
    // TODO: add read_step
  }
  state->change(TestState::DONE);
}

static void concurrent_writer(void* arg) {
  TestState* state = static_cast<TestState*>(arg);
  uint32_t k = state->next_writer_++ % ConcurrentTest::K;
  state->t_.concurrent_write_step(k);
  state->adjust_pending_writers(-1);
}


static void RunConcurrentInsert(int write_parallelism = 4) {
  common::ThreadPoolExecutor executor_;
  executor_.init("skiplist_test", write_parallelism, write_parallelism, 60 * 1000);
  common::RandomGenerator rnd;
  const int N = 100;
  const int kSize = 10;
  for (int i = 0; i < N; i++) {
    TestState* state = new TestState();
    executor_.execute(std::bind(concurrent_reader, state));
    state->wait(TestState::RUNNING);
    int k = 0;
    for (k = 0; k < kSize; k += write_parallelism) {
      state->next_writer_ = rnd.next();
      state->adjust_pending_writers(write_parallelism);
      for (int p = 0; p < write_parallelism; ++p) {
        executor_.execute(std::bind(concurrent_writer, state));
      }
      state->wait_for_pending_writers();
    }
    int count = state->t_.scan();
    ASSERT_EQ(k, count);
    state->quit_flag_.store(true, std::memory_order_release);
    state->wait(TestState::DONE);
    delete state;
  }
}

TEST_F(InlineSkipTest, DISABLED_ConcurrentInsert2) { RunConcurrentInsert(2); }
TEST_F(InlineSkipTest, DISABLED_ConcurrentInsert3) { RunConcurrentInsert(4); }

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
