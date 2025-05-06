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
// Created by Ping Xu(haibarapink@gmail.com) on 2025/2/9.
//
#pragma once

#include "common/lang/mutex.h"
#include "common/sys/rc.h"
#include "oblsm/util/ob_file_writer.h"

namespace oceanbase {

/**
 * @struct WalRecord
 * @brief A structure representing a record in the Write-Ahead Log (WAL).
 * Each record contains a sequence number, a key, and a value.
 */
struct WalRecord
{
  /** Sequence number of the record, used to identify the order of records. */
  uint64_t seq;
  /** The key associated with the record. */
  std::string key;
  /** The value associated with the record. */
  std::string val;

  /**
   * @brief Parameterized constructor to create a new WalRecord object.
   *
   * @param s The sequence number.
   * @param k The key.
   * @param v The value.
   */
  WalRecord(uint64_t s, std::string k, std::string v) : seq(s), key(std::move(k)), val(std::move(v)) {}
};

/**
 * @class Wal
 * @brief A class responsible for memtable recovery and Write-Ahead Log (WAL) operations.
 * This class enables writing data to a file using the WAL mechanism and recovering data from a previously written WAL
 * file. The WAL format ensures that data modifications are logged before being applied to the main data store,
 * providing durability in case of system failures.
 *
 * ### Data Serialization Format:
 * The data is serialized as follows:
 * - Each entry in the WAL consists of a key-value pair.
 * - The data format is:
 *   - **Sequence Number (uint64_t)**: A 8-byte value representing the sequence of logs.
 *   - **Key Length (size_t)**: A value representing the length of the key.
 *   - **Key (string)**: The actual key, as a string.
 *   - **Value Length (size_t)**: A value representing the length of the value.
 *   - **Value (string)**: The actual value, as a string.
 *
 * The data is written to the file in the order: key length, key, value length, value.
 * After writing the data, the system performs a `flush()` operation to ensure the data is persisted.
 */
class WAL
{
public:
  /**
   * @brief Default constructor for the Wal class.
   */
  WAL() {}

  /**
   * @brief Destructor for the Wal class.
   * Ensures that the file writer is closed when the Wal object is destroyed.
   */
  ~WAL() = default;

  /**
   * @brief Opens the WAL file for writing.
   * This function initializes the file writer and prepares the WAL file for appending data.
   * If the file cannot be opened, an error code is returned.
   *
   * @param filename The name of the WAL file to write logs.
   * @return `RC::SUCCESS` if the file was successfully opened, or an error code if it failed.
   */
  RC open(const std::string &filename) { return RC::UNIMPLEMENTED; }

  /**
   * @brief Recovers data from a specified WAL file.
   *
   * This function reads the given WAL file, extracts key-value pairs, and stores them in the provided vector.
   * It also returns the total number of records read from the WAL.
   *
   * @param wal_file The name of the WAL file to recover from.
   * @param wal_records A reference to a vector where the WalRecord objects will be stored.
   * @return `RC::SUCCESS` if recovery is successful, or an error code if it fails.
   */
  RC recover(const std::string &wal_file, std::vector<WalRecord> &wal_records);

  /**
   * @brief Writes a key-value pair to the WAL.
   *
   * This function serializes the key-value pair and appends it to the WAL file.
   *
   * @param seq The sequence number of the record.
   * @param key The key to write.
   * @param val The value associated with the key.
   * @return `RC::SUCCESS` if the write operation is successful, or an error code if it fails.
   */
  RC put(uint64_t seq, std::string_view key, std::string_view val);

  /**
   * @brief Synchronizes the WAL to disk.
   * Forces any buffered data in the WAL to be written to the underlying storage.
   *
   * @return `RC::SUCCESS` if the sync operation is successful, or an error code if it fails.
   */
  RC sync() { return RC::UNIMPLEMENTED; }

  const string &filename() const { return filename_; }

private:
  string filename_;
};
}  // namespace oceanbase
