/* Copyright (c) 2021-2022 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by wangyunlai on 2024/01/31
//

#define private public
#define protected public

#include "gtest/gtest.h"
#include "storage/clog/log_handler.h"
#include "storage/clog/log_replayer.h"
#include "common/thread/thread_pool_executor.h"

using namespace std;
using namespace common;

class TestLogReplayer : public LogReplayer
{
public:
  virtual ~TestLogReplayer() = default;
  RC replay(const LogEntry &entry) override
  {
    count_++;
    return RC::SUCCESS;
  }

  int64_t count() const { return count_; }

private:
  int64_t count_ = 0;
};

TEST(LogHandler, empty)
{
  // specific an empty directory and test LogHandler start/stop and so on
  const char *path = "test_log_handler";
  LogHandler handler;
  ASSERT_EQ(RC::SUCCESS, handler.init(path));
  ASSERT_EQ(RC::SUCCESS, handler.start());
  ASSERT_EQ(RC::SUCCESS, handler.stop());
  ASSERT_EQ(RC::SUCCESS, handler.wait());

  filesystem::remove_all(path);
}

TEST(LogHandler, test_append_and_wait)
{
  // specific an empty directory and call LogHandler::append 10000 times and stop it
  const char *path = "test_log_handler";
  filesystem::remove_all(path);

  LogHandler handler;
  ASSERT_EQ(RC::SUCCESS, handler.init(path));
  TestLogReplayer replayer;
  ASSERT_EQ(RC::SUCCESS, handler.replay(replayer, 0));
  ASSERT_EQ(RC::SUCCESS, handler.start());

  int times = 10000;
  for (int i = 0; i < times; ++i) {
    LSN lsn = 0;
    ASSERT_EQ(handler.append(lsn, LogModule::Id::BUFFER_POOL, unique_ptr<char[]>(new char[10]), 10), RC::SUCCESS);

    if (i % 999 == 0) {
      ASSERT_EQ(RC::SUCCESS, handler.wait_lsn(i));
    }
  }

  ASSERT_EQ(RC::SUCCESS, handler.stop());
  ASSERT_EQ(RC::SUCCESS, handler.wait());

  filesystem::remove_all(path);
}

TEST(LogHandler, test_replay)
{
  // create an empty directory and init a LogHandler and then test replay
  // We will do nothing as there is no log file in the directory
  const char *path = "test_log_handler";
  filesystem::remove_all(path);

  LogHandler handler;
  TestLogReplayer replayer;
  ASSERT_EQ(RC::SUCCESS, handler.init(path));
  ASSERT_EQ(RC::SUCCESS, handler.replay(replayer, 0));
  ASSERT_EQ(0, replayer.count());
  ASSERT_EQ(RC::SUCCESS, handler.start());

  const int times = 10010;
  for (int i = 0; i < times; ++i) {
    LogEntry entry;
    LSN lsn = 0;
    ASSERT_EQ(handler.append(lsn, LogModule::Id::BUFFER_POOL, unique_ptr<char[]>(new char[10]), 10), RC::SUCCESS);
  }

  ASSERT_EQ(RC::SUCCESS, handler.stop());
  ASSERT_EQ(RC::SUCCESS, handler.wait());

  int count = 0;
  auto log_entry_counter = [&count](LogEntry &) -> RC {
    count++;
    return RC::SUCCESS;
  };
  ASSERT_EQ(RC::SUCCESS, handler.iterate(log_entry_counter, 0));
  ASSERT_EQ(count, times);

  LOG_INFO("write %d log entries done and restart log handle", times);

  LogHandler handler2;
  ASSERT_EQ(RC::SUCCESS, handler2.init(path));
  ASSERT_EQ(RC::SUCCESS, handler2.replay(replayer, 0));
  ASSERT_EQ(times, replayer.count());
  ASSERT_EQ(handler2.current_lsn(), times);

  ASSERT_EQ(RC::SUCCESS, handler2.start());

  const int times2 = 2030;
  for (int i = 0; i < times2; ++i) {
    LSN lsn = 0;
    ASSERT_EQ(handler2.append(lsn, LogModule::Id::BUFFER_POOL, unique_ptr<char[]>(new char[10]), 10), RC::SUCCESS);
  }

  ASSERT_EQ(handler2.current_lsn(), times2 + times);
  
  ASSERT_EQ(RC::SUCCESS, handler2.stop());
  ASSERT_EQ(RC::SUCCESS, handler2.wait());
  
  count = 0;
  ASSERT_EQ(RC::SUCCESS, handler2.iterate(log_entry_counter, 0));
  ASSERT_EQ(count, times + times2);

  LogHandler handler3;
  ASSERT_EQ(RC::SUCCESS, handler3.init(path));
  TestLogReplayer replayer3;
  ASSERT_EQ(RC::SUCCESS, handler3.replay(replayer3, 2000));
  ASSERT_EQ(times2 + times - 2000 + 1, replayer3.count());
  ASSERT_EQ(handler3.current_lsn(), times2 + times);

  // filesystem::remove_all(path);
}

TEST(LogHandler, multi_thread)
{
  const char *directory = "test_log_handler_multi_thread";
  filesystem::remove_all(directory);

  LogHandler handler;
  TestLogReplayer replayer;
  ASSERT_EQ(RC::SUCCESS, handler.init(directory));
  ASSERT_EQ(RC::SUCCESS, handler.replay(replayer, 0));
  ASSERT_EQ(RC::SUCCESS, handler.start());

  const int times = 200000;
  ThreadPoolExecutor executor;
  ASSERT_EQ(0, executor.init("TestLogHandler", 4, 4, 60*1000));

  for (int i = 0; i < times; ++i) {
    ASSERT_EQ(0, executor.execute([&handler]() -> void {
      LSN lsn = 0;
      ASSERT_EQ(handler.append(lsn, LogModule::Id::BUFFER_POOL, unique_ptr<char[]>(new char[10]), 10), RC::SUCCESS);
    }));
  }

  ASSERT_EQ(0, executor.shutdown());
  ASSERT_EQ(0, executor.await_termination());
  ASSERT_EQ(RC::SUCCESS, handler.stop());
  ASSERT_EQ(RC::SUCCESS, handler.wait());
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  LoggerFactory::init_default(string(argv[0]) + ".log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}