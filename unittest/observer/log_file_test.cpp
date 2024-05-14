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

#include <span>

#include "gtest/gtest.h"

#define private public
#define protected public

#include "common/log/log.h"
#include "storage/clog/log_file.h"
#include "storage/clog/log_entry.h"
#include "storage/clog/log_module.h"

using namespace std;
using namespace common;

TEST(LogFileWriter, basic)
{
  const char *filename = "test_log_file_writer.log";

  // test LogFileWriter open, close, valid
  LogFileWriter writer;
  LSN           end_lsn = 1000 - 1;
  ASSERT_EQ(RC::SUCCESS, writer.open(filename, end_lsn));
  ASSERT_TRUE(writer.valid());
  ASSERT_FALSE(writer.full());
  ASSERT_EQ(RC::SUCCESS, writer.close());

  // test LogFileWriter write
  ASSERT_EQ(RC::SUCCESS, writer.open(filename, end_lsn));

  LogEntry entry;

  for (LSN lsn = 1; lsn <= end_lsn - 1; ++lsn) {
    vector<char> data(10);
    ASSERT_EQ(entry.init(lsn, LogModule::Id::BUFFER_POOL, std::move(data)), RC::SUCCESS);
    char buf[10];
    memcpy(buf, entry.data(), 10);
    ASSERT_EQ(RC::SUCCESS, writer.write(entry));
    ASSERT_TRUE(writer.valid());
    ASSERT_FALSE(writer.full());
  }

  // test LogFileWriter full
  vector<char> data(10);
  ASSERT_EQ(entry.init(end_lsn, LogModule::Id::BUFFER_POOL, std::move(data)), RC::SUCCESS);
  ASSERT_EQ(RC::SUCCESS, writer.write(entry));
  ASSERT_TRUE(writer.valid());
  ASSERT_TRUE(writer.full());

  // test LogFileWriter write smaller lsn log
  data.resize(10);
  ASSERT_EQ(entry.init(end_lsn - 100, LogModule::Id::BUFFER_POOL, std::move(data)), RC::SUCCESS);
  ASSERT_NE(RC::SUCCESS, writer.write(entry));

  data.resize(10);
  writer.close();
  ASSERT_EQ(RC::SUCCESS, writer.open(filename, end_lsn));
  ASSERT_EQ(entry.init(end_lsn + 1, LogModule::Id::BUFFER_POOL, std::move(data)), RC::SUCCESS);
  ASSERT_NE(RC::SUCCESS, writer.write(entry));
  ASSERT_TRUE(writer.full());

  filesystem::remove(filename);
}

TEST(LogFileReader, basic)
{
  const char *log_file = "test_log_file_reader.log";

  filesystem::remove(log_file);

  LogFileWriter writer;
  LSN           end_lsn = 1000 - 1;
  ASSERT_EQ(RC::SUCCESS, writer.open(log_file, end_lsn));
  ASSERT_TRUE(writer.valid());

  LogEntry entry;

  for (LSN lsn = 1; lsn <= end_lsn; ++lsn) {
    vector<char> data(10);
    ASSERT_EQ(entry.init(lsn, LogModule::Id::BUFFER_POOL, std::move(data)), RC::SUCCESS);
    ASSERT_EQ(RC::SUCCESS, writer.write(entry));
  }

  writer.close();

  LogFileReader reader;
  ASSERT_EQ(RC::SUCCESS, reader.open(log_file));

  int  count    = 0;
  auto callback = [&count](LogEntry &entry) -> RC {
    LOG_DEBUG("entry=%s", entry.to_string().c_str());
    count++;
    return RC::SUCCESS;
  };

  count = 0;
  ASSERT_EQ(RC::SUCCESS, reader.iterate(callback));
  ASSERT_EQ(end_lsn, count);

  LSN start_lsn = 500;
  count         = 0;
  ASSERT_EQ(RC::SUCCESS, reader.iterate(callback, start_lsn));
  ASSERT_EQ(end_lsn - start_lsn + 1, count);

  start_lsn = end_lsn + 100;
  count     = 0;
  ASSERT_EQ(RC::SUCCESS, reader.iterate(callback, start_lsn));
  ASSERT_EQ(0, count);

  filesystem::remove(log_file);
}

TEST(LogFileReadWrite, test_read_write)
{
  const char *log_file = "test_log_file_read_write.log";

  filesystem::remove(log_file);

  LogFileWriter writer;
  LSN           end_lsn = 1000 - 1;
  ASSERT_EQ(RC::SUCCESS, writer.open(log_file, end_lsn));
  ASSERT_TRUE(writer.valid());

  LogEntry entry;

  LSN one_lsn = end_lsn - 500;
  for (LSN lsn = 1; lsn <= one_lsn; ++lsn) {
    vector<char> data(10);
    ASSERT_EQ(entry.init(lsn, LogModule::Id::BUFFER_POOL, std::move(data)), RC::SUCCESS);
    ASSERT_EQ(RC::SUCCESS, writer.write(entry));
  }

  writer.close();

  LogFileReader reader;
  ASSERT_EQ(RC::SUCCESS, reader.open(log_file));

  int  count    = 0;
  auto callback = [&count](LogEntry &entry) -> RC {
    count++;
    return RC::SUCCESS;
  };

  count = 0;
  ASSERT_EQ(RC::SUCCESS, reader.iterate(callback));
  ASSERT_EQ(one_lsn, count);

  writer.close();
  reader.close();

  ASSERT_EQ(RC::SUCCESS, writer.open(log_file, end_lsn));

  for (LSN i = one_lsn + 1; i <= end_lsn; i++) {
    vector<char> data(10);
    ASSERT_EQ(RC::SUCCESS, entry.init(i, LogModule::Id::BUFFER_POOL, std::move(data)));
    ASSERT_EQ(RC::SUCCESS, writer.write(entry));
  }

  ASSERT_EQ(RC::SUCCESS, reader.open(log_file));
  count = 0;
  ASSERT_EQ(RC::SUCCESS, reader.iterate(callback, 0));
  ASSERT_EQ(end_lsn, count);

  // filesystem::remove(log_file);
}

TEST(LogFileManager, get_lsn_from_filename)
{
  const char *file_prefix = LogFileManager::file_prefix_;
  const char *file_suffix = LogFileManager::file_suffix_;

  vector<LSN>  test_lsn{0, 1, 10, 100, 1000, 10000, 100000, 1000000};
  stringstream filename_ss;
  for (auto lsn : test_lsn) {
    filename_ss.str("");
    filename_ss << file_prefix << lsn << file_suffix;
    LSN lsn_in_filename = 0;
    ASSERT_EQ(RC::SUCCESS, LogFileManager::get_lsn_from_filename(filename_ss.str().c_str(), lsn_in_filename));
    ASSERT_EQ(lsn, lsn_in_filename);
  }

  // filename with invalid prefix or invalid suffix
  LSN lsn_in_filename = 0;
  ASSERT_NE(RC::SUCCESS, LogFileManager::get_lsn_from_filename("invalid_prefix_1000.log", lsn_in_filename));
  ASSERT_NE(
      RC::SUCCESS, LogFileManager::get_lsn_from_filename(string(file_prefix) + "1000.invalid_suffix", lsn_in_filename));
}

TEST(LogFileManager, init_not_exists)
{
  const char *directory                 = "not_exists/not_exists2";
  int         max_entry_number_per_file = 1000;

  LogFileManager manager;
  ASSERT_EQ(RC::SUCCESS, manager.init(directory, max_entry_number_per_file));
  ASSERT_TRUE(filesystem::is_directory(directory));

  vector<string> files;
  ASSERT_EQ(RC::SUCCESS, manager.list_files(files, 0));
  ASSERT_EQ(0, files.size());

  ASSERT_TRUE(filesystem::remove_all(directory));
}

TEST(LogFileManager, init_empty_directory)
{
  const char *directory                 = "empty_directory";
  int         max_entry_number_per_file = 1000;

  ASSERT_TRUE(filesystem::create_directory(directory));

  LogFileManager manager;
  ASSERT_EQ(RC::SUCCESS, manager.init(directory, max_entry_number_per_file));
  ASSERT_TRUE(filesystem::is_directory(directory));

  vector<string> files;
  ASSERT_EQ(RC::SUCCESS, manager.list_files(files, 0));
  ASSERT_EQ(0, files.size());

  ASSERT_TRUE(filesystem::remove_all(directory));
}

TEST(LogFileManager, init_with_files)
{
  const char *directory                 = "init_with_files";
  int         max_entry_number_per_file = 1000;

  filesystem::remove_all(directory);

  ASSERT_TRUE(filesystem::create_directory(directory));

  LSN            lsns[] = {1000, 2000, 3000};
  vector<string> files;
  for (LSN lsn : lsns) {
    stringstream filename_ss;
    string       filename = string(LogFileManager::file_prefix_) + to_string(lsn) + LogFileManager::file_suffix_;
    files.push_back(filename);
    filename_ss << directory << "/" << filename;
    ofstream ofs(filename_ss.str());
    ofs.close();
  }

  LogFileManager manager;
  ASSERT_EQ(RC::SUCCESS, manager.init(directory, max_entry_number_per_file));
  ASSERT_TRUE(filesystem::is_directory(directory));

  vector<string> result_files;
  ASSERT_EQ(RC::SUCCESS, manager.list_files(result_files, 0));
  ASSERT_EQ(files.size(), result_files.size());
  for (int i = 0; i < static_cast<int>(files.size()); ++i) {
    ASSERT_EQ(files[i], filesystem::path(result_files[i]).filename());
  }

  ASSERT_EQ(RC::SUCCESS, manager.list_files(result_files, 100));
  ASSERT_EQ(files.size(), result_files.size());

  ASSERT_EQ(RC::SUCCESS, manager.list_files(result_files, 1000));
  ASSERT_EQ(files.size(), result_files.size());

  ASSERT_EQ(RC::SUCCESS, manager.list_files(result_files, 1500));
  ASSERT_EQ(3, result_files.size());

  ASSERT_EQ(RC::SUCCESS, manager.list_files(result_files, 2000));
  ASSERT_EQ(2, result_files.size());

  ASSERT_EQ(RC::SUCCESS, manager.list_files(result_files, 2500));
  ASSERT_EQ(2, result_files.size());

  ASSERT_EQ(RC::SUCCESS, manager.list_files(result_files, 3000));
  ASSERT_EQ(1, result_files.size());

  ASSERT_EQ(RC::SUCCESS, manager.list_files(result_files, 3010));
  ASSERT_EQ(1, result_files.size());

  ASSERT_EQ(RC::SUCCESS, manager.list_files(result_files, 4000));
  ASSERT_EQ(0, result_files.size());

  ASSERT_EQ(RC::SUCCESS, manager.list_files(result_files, 5000));
  ASSERT_EQ(0, result_files.size());

  ASSERT_TRUE(filesystem::remove_all(directory));
}

TEST(LogFileManager, last_file)
{
  // create an empty directory and try to open last file
  const char *directory                 = "last_file";
  int         max_entry_number_per_file = 1000;

  filesystem::remove_all(directory);

  LogFileManager manager;
  ASSERT_EQ(RC::SUCCESS, manager.init(directory, max_entry_number_per_file));
  ASSERT_TRUE(filesystem::is_directory(directory));

  LogFileWriter writer;
  ASSERT_EQ(RC::SUCCESS, manager.last_file(writer));
  ASSERT_TRUE(writer.valid());

  // test the lsn of the filename of the writer
  LSN              lsn = 0;
  filesystem::path file_path(writer.filename());
  ASSERT_EQ(RC::SUCCESS, LogFileManager::get_lsn_from_filename(file_path.filename(), lsn));
  ASSERT_EQ(0, lsn);

  ASSERT_TRUE(filesystem::remove_all(directory));

  // create a directory with some files and try to open last file
  ASSERT_TRUE(filesystem::create_directory(directory));
  LSN            lsns[] = {1000, 2000, 3000};
  vector<string> files;
  for (LSN lsn : lsns) {
    stringstream filename_ss;
    string       filename = string(LogFileManager::file_prefix_) + to_string(lsn) + LogFileManager::file_suffix_;
    files.push_back(filename);
    filename_ss << directory << "/" << filename;
    ofstream ofs(filename_ss.str());
    ofs.close();
  }

  ASSERT_EQ(RC::SUCCESS, manager.init(directory, max_entry_number_per_file));
  ASSERT_TRUE(filesystem::is_directory(directory));

  ASSERT_EQ(RC::SUCCESS, manager.last_file(writer));
  ASSERT_TRUE(writer.valid());
  ASSERT_EQ(files[2], filesystem::path(writer.filename()).filename());

  filesystem::remove_all(directory);
}

TEST(LogFileManager, next_file)
{
  // create an empty directory and try to open next file
  const char *directory                 = "next_file";
  int         max_entry_number_per_file = 1000;

  filesystem::remove_all(directory);

  LogFileManager manager;
  ASSERT_EQ(RC::SUCCESS, manager.init(directory, max_entry_number_per_file));
  ASSERT_TRUE(filesystem::is_directory(directory));

  LogFileWriter writer;
  ASSERT_EQ(RC::SUCCESS, manager.next_file(writer));
  ASSERT_TRUE(writer.valid());

  // test the lsn of the filename of the writer
  LSN lsn = 0;
  ASSERT_EQ(RC::SUCCESS, LogFileManager::get_lsn_from_filename(filesystem::path(writer.filename()).filename(), lsn));
  ASSERT_EQ(0, lsn);

  ASSERT_TRUE(filesystem::remove_all(directory));

  // create a directory with some files and try to open next file
  ASSERT_TRUE(filesystem::create_directory(directory));
  LSN            lsns[] = {1000, 2000, 3000};
  vector<string> files;
  for (LSN lsn : lsns) {
    stringstream filename_ss;
    string       filename = string(LogFileManager::file_prefix_) + to_string(lsn) + LogFileManager::file_suffix_;
    files.push_back(filename);
    filename_ss << directory << "/" << filename;
    ofstream ofs(filename_ss.str());
    ofs.close();
  }

  ASSERT_EQ(RC::SUCCESS, manager.init(directory, max_entry_number_per_file));
  ASSERT_TRUE(filesystem::is_directory(directory));

  ASSERT_EQ(RC::SUCCESS, manager.next_file(writer));
  ASSERT_TRUE(writer.valid());
  ASSERT_EQ(RC::SUCCESS, LogFileManager::get_lsn_from_filename(filesystem::path(writer.filename()).filename(), lsn));
  ASSERT_EQ(4000, lsn);

  writer.close();
  filesystem::remove_all(directory);
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  LoggerFactory::init_default("log_file_test.log", LOG_LEVEL_TRACE);
  return RUN_ALL_TESTS();
}
