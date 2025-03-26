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
#include "common/lang/string.h"
#include "common/lang/string_view.h"
#include "common/lang/memory.h"
#include "common/sys/rc.h"

namespace oceanbase {

/**
 * @class ObFileWriter
 * @brief A utility class for writing data to files.
 *
 * The `ObFileWriter` class provides a convenient interface for writing data to a file.
 * It supports creating and opening files for writing, appending data to existing files,
 * and flushing buffered data to disk. The class ensures proper resource management by
 * providing methods for explicitly closing the file.
 * TODO: use posix
 */
class ObFileWriter
{
public:
  /**
   * @brief Constructs an `ObFileWriter` object with the specified file name and mode.
   *
   * The constructor initializes the file writer with the given file name. The file is not opened
   * until `open_file()` is called. The `append` parameter determines whether the file should
   * be opened in append mode or overwrite mode.
   *
   * @param filename The name of the file to write to.
   * @param append Whether to open the file in append mode (default: `false`).
   */
  ObFileWriter(const string &filename, bool append = false) : filename_(filename), append_(append) {}

  ~ObFileWriter();

  /**
   * @brief Opens the file for writing.
   *
   * This method attempts to open the file specified during the construction of the object.
   * If the file is successfully opened, further write operations can be performed.
   *
   * @return An RC (return code) indicating the success or failure of the operation.
   * @note If the file cannot be opened (e.g., due to permission issues), an error code is returned.
   */
  RC open_file();

  /**
   * @brief Closes the file if it is currently open.
   *
   * This method releases the resources associated with the file. After calling this method,
   * the file can no longer be written to until it is reopened.
   */
  void close_file();

  /**
   * @brief Writes data to the file.
   *
   * Appends the provided data to the file. If the file is not open, the operation will fail.
   *
   * @param data The data to write to the file, provided as a `string_view`.
   * @return An RC (return code) indicating the success or failure of the operation.
   */
  RC write(const string_view &data);

  /**
   * @brief Flushes buffered data to disk.
   *
   * Ensures that all buffered data is written to the file system. This method is useful
   * for ensuring data integrity in cases where the program may terminate unexpectedly.
   *
   * @return An RC (return code) indicating the success or failure of the flush operation.
   */
  RC flush();

  /**
   * @brief Checks if the file is currently open.
   *
   * @return `true` if the file is open, `false` otherwise.
   */
  bool is_open() const { return file_.is_open(); }

  /**
   * @brief Returns the name of the file being written to.
   *
   * @return A string containing the file name.
   */
  string file_name() const { return filename_; }

  /**
   * @brief Creates a new `ObFileWriter` instance.
   *
   * This static factory method constructs a new `ObFileWriter` object with the specified
   * file name and append mode.
   *
   * @param filename The name of the file to write to.
   * @param append Whether to open the file in append mode (default: `false`).
   * @return A `unique_ptr` to the created `ObFileWriter` object.
   */
  static unique_ptr<ObFileWriter> create_file_writer(const string &filename, bool append);

private:
  /**
   * @brief The name of the file to be written to.
   */
  string filename_;

  /**
   * @brief Indicates whether the file should be opened in append mode.
   *
   * If `true`, data will be appended to the existing file. If `false`, the existing file (if any)
   * will be overwritten when the file is opened.
   */
  bool append_;

  /**
   * @brief The file stream used for writing data.
   */
  ofstream file_;
};
}  // namespace oceanbase
