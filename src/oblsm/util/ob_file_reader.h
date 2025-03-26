/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/lang/fstream.h"
#include "common/lang/memory.h"
#include "common/lang/sstream.h"
#include "common/lang/string.h"
#include "common/sys/rc.h"
#include "common/lang/mutex.h"

namespace oceanbase {

/**
 * @class ObFileReader
 * @brief A utility class for reading files in an efficient manner.
 *
 * The `ObFileReader` class provides a simple interface for reading files. It allows
 * opening, closing, and reading specific portions of a file while also exposing
 * the file size for external use. This class is intended for use in scenarios where
 * file-based data storage, such as SSTables, is accessed.
 */
class ObFileReader
{
public:
  /**
   * @brief Constructs an `ObFileReader` object with the specified file name.
   *
   * The file name is stored internally, but the file is not opened until
   * `open_file()` is explicitly called.
   *
   * @param filename The name of the file to be read.
   */
  ObFileReader(const string &filename) : filename_(filename) {}

  ~ObFileReader();

  /**
   * @brief Opens the file for reading.
   *
   * This method attempts to open the file specified during the construction
   * of the object. If the file is successfully opened, the internal file descriptor
   * (`fd_`) is updated.
   *
   * @return An RC (return code) indicating the success or failure of the operation.
   */
  RC open_file();

  /**
   * @brief Closes the file if it is currently open.
   *
   * This method releases the file descriptor (`fd_`) associated with the file.
   */
  void close_file();

  /**
   * @brief Reads a portion of the file from a specified position.
   *
   * This method reads `size` bytes starting from position `pos` in the file.
   *
   * @param pos The starting position (offset) in the file.
   * @param size The number of bytes to read.
   * @return A string containing the requested portion of the file's data.
   */
  string read_pos(uint32_t pos, uint32_t size);

  /**
   * @brief Returns the size of the file.
   *
   * This method retrieves the size of the file in bytes. It relies on the file
   * being successfully opened.
   *
   * @return The size of the file in bytes.
   */
  uint32_t file_size();

  /**
   * @brief Creates a new `ObFileReader` instance.
   *
   * This static factory method constructs a new `ObFileReader` object and
   * initializes it with the specified file name.
   *
   * @param filename The name of the file to be read.
   * @return A `unique_ptr` to the created `ObFileReader` object.
   */
  static unique_ptr<ObFileReader> create_file_reader(const string &filename);

private:
  /**
   * @brief The name of the file to be read.
   *
   * This string stores the file name specified during the construction of
   * the `ObFileReader` object.
   */
  string filename_;

  /**
   * @brief The file descriptor for the currently opened file.
   *
   * This integer represents the file descriptor used for reading the file.
   * If no file is open, it is set to `-1`.
   */
  int fd_ = -1;
};

}  // namespace oceanbase
