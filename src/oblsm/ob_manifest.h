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
// Created by Ping Xu(haibarapink@gmail.com) on 2025/1/24.
//
#pragma once

#include "json/json.h"
#include "common/lang/filesystem.h"
#include "common/lang/string_view.h"
#include "common/log/log.h"
#include "common/sys/rc.h"
#include "oblsm/ob_lsm_define.h"
#include "oblsm/util/ob_file_writer.h"
#include "oblsm/util/ob_file_reader.h"

namespace oceanbase {

/**
 * @brief A utility class to convert between JSON and user-defined types.
 *
 * Provides template methods for converting an object to JSON and vice versa.
 * Specialized template functions exist for specific types like `CompactionType`.
 */
class JsonConverter
{
public:
  /**
   * @brief Converts an object of type T to a JSON value.
   *
   * This function uses the `to_json()` method of the type T to convert it to JSON.
   *
   * @tparam T The type of the object.
   * @param t The object to be converted.
   * @return The JSON value representing the object.
   */
  template <typename T>
  static Json::Value to_json(const T &t)
  {
    return t.to_json();
  }

  /**
   * @brief Converts a JSON value to an object of type T.
   *
   * This function uses the `from_json()` method of the type T to convert the JSON value.
   *
   * @tparam T The type of the object.
   * @param v The JSON value to be converted.
   * @param t The object that will be populated from the JSON value.
   * @return An RC indicating the result of the operation.
   */
  template <typename T>
  static RC from_json(const Json::Value &v, T &t)
  {
    RC rc = t.from_json(v);
    return rc;
  }
};

/**
 * @brief Specialization of `to_json()` for `CompactionType`.
 *
 * Converts a `CompactionType` value to its integer representation in JSON format.
 *
 * @param type The `CompactionType` value to convert.
 * @return A JSON value representing the integer value of `CompactionType`.
 */
template <>
inline Json::Value JsonConverter::to_json<CompactionType>(const CompactionType &type)
{
  int         type_as_int = static_cast<int>(type);
  Json::Value res{type_as_int};
  return res;
}

/**
 * @brief Specialization of `from_json()` for `CompactionType`.
 *
 * Converts a JSON value representing an integer into a `CompactionType`.
 *
 * @param v The JSON value to convert.
 * @param type The `CompactionType` value that will be populated from the JSON value.
 * @return An RC indicating the result of the operation.
 */
template <>
inline RC JsonConverter::from_json<CompactionType>(const Json::Value &v, CompactionType &type)
{
  int type_as_int = -1;
  if (v.isInt()) {
    type_as_int = v.asInt();
  } else {
    return RC::INVALID_ARGUMENT;
  }

  type = static_cast<CompactionType>(type_as_int);
  return RC::SUCCESS;
}

/**
 * @brief Struct representing SSTable information in a manifest.
 *
 * This struct contains metadata about an SSTable, including its ID and level.
 */
struct ObManifestSSTableInfo
{
  int sstable_id;  ///< The ID of the SSTable.
  int level;       ///< The level of the SSTable.

  /**
   * @brief Constructor to initialize with given SSTable ID and level.
   *
   * @param sstable_id_ The SSTable ID.
   * @param level_ The SSTable level.
   */
  ObManifestSSTableInfo(int sstable_id_, int level_) : sstable_id(sstable_id_), level(level_) {}

  /** Default constructor with default values (0). */
  ObManifestSSTableInfo() : sstable_id(0), level(0) {}

  /**
   * @brief Equality operator for comparing two `ObManifestSSTableInfo` objects.
   *
   * @param rhs The other `ObManifestSSTableInfo` to compare with.
   * @return `true` if the two objects are equal, `false` otherwise.
   */
  bool operator==(const ObManifestSSTableInfo &rhs) const { return sstable_id == rhs.sstable_id && level == rhs.level; }

  /**
   * @brief Converts the `ObManifestSSTableInfo` object to a JSON value.
   *
   * @return The JSON value representing the SSTable info.
   */
  Json::Value to_json() const
  {
    Json::Value v;
    v["sstable_id"] = sstable_id;
    v["level"]      = level;
    return v;
  }

  /**
   * @brief Populates the `ObManifestSSTableInfo` object from a JSON value.
   *
   * @param v The JSON value to populate the object with.
   * @return An RC indicating the result of the operation.
   */
  RC from_json(const Json::Value &v);
};

/**
 * @brief Represents a record in the manifest, used for compaction operations.
 *
 * Contains information about SSTables that have been added or deleted during a compaction.
 */
class ObManifestCompaction
{
public:
  CompactionType                     compaction_type;      ///< The type of compaction (e.g., level)
  std::vector<ObManifestSSTableInfo> deleted_tables;       ///< List of deleted SSTables.
  std::vector<ObManifestSSTableInfo> added_tables;         ///< List of added SSTables.
  uint64_t                           sstable_sequence_id;  ///< Sequence ID for SSTables.
  uint64_t                           seq_id;               ///< Sequence ID for the record.

  static string record_type() { return "ObManifestCompaction"; }

  bool operator==(const ObManifestCompaction &rhs) const
  {
    return compaction_type == rhs.compaction_type && deleted_tables == rhs.deleted_tables &&
           added_tables == rhs.added_tables && sstable_sequence_id == rhs.sstable_sequence_id && seq_id == rhs.seq_id;
  }

  /**
   * @brief Converts the `ObManifestCompaction` to a JSON value.
   *
   * @return The JSON value representing the record.
   */
  Json::Value to_json() const;

  /**
   * @brief Populates the `ObManifestCompaction` object from a JSON value.
   *
   * @param v The JSON value to populate the object with.
   * @return An RC indicating the result of the operation.
   */
  RC from_json(const Json::Value &v);
};

/**
 * @brief Represents a snapshot of the manifest.
 *
 * Contains information about the current state of the SSTables.
 */
class ObManifestSnapshot
{
public:
  std::vector<std::vector<uint64_t>> sstables;         ///< A list of SSTable IDs grouped by levels.
  uint64_t                           seq;              ///< The sequence ID for the snapshot.
  uint64_t                           sstable_id;       ///< The ID of the SSTable.
  CompactionType                     compaction_type;  ///< The type of compaction.

  static string record_type() { return "ObManifestSnapshot"; }

  bool operator==(const ObManifestSnapshot &rhs) const
  {
    return sstables == rhs.sstables && seq == rhs.seq && sstable_id == rhs.sstable_id &&
           compaction_type == rhs.compaction_type;
  }

  /**
   * @brief Converts the `ObManifestSnapshot` to a JSON value.
   *
   * @return The JSON value representing the snapshot.
   */
  Json::Value to_json() const;

  /**
   * @brief Populates the `ObManifestSnapshot` object from a JSON value.
   *
   * @param v The JSON value to populate the object with.
   * @return An RC indicating the result of the operation.
   */
  RC from_json(const Json::Value &v);
};

/**
 * @brief Represents a new memtable of ObLsm.
 *
 * It is used to recover the latest memtable.
 */
class ObManifestNewMemtable
{
public:
  uint64_t memtable_id;  ///< The id of memtable and it is used to find the WAL file of memtable.(e.g., memtable=1, WAL
                         ///< file = 1.wal)

  static string record_type() { return "ObManifestNewMemtable"; }

  bool operator==(const ObManifestNewMemtable &rhs) const { return memtable_id == rhs.memtable_id; }

  /**
   * @brief Converts the `ObManifestNewMemtable` to a JSON value.
   *
   * @return The JSON value representing the new memtable.
   */
  Json::Value to_json() const;

  /**
   * @brief Populates the `ObManifestNewMemtable` object from a JSON value.
   *
   * @param v The JSON value to populate the object with.
   * @return An RC indicating the result of the operation.
   */
  RC from_json(const Json::Value &v);
};

/**
 * @brief A class that manages the manifest file, including reading and writing records and snapshots.
 *
 * This class handles the reading and writing of manifest records and snapshots, ensuring the data is serialized to
 * JSON format and stored in a file. The manifest file is used to track SSTable changes during compaction and recovery.
 */
class ObManifest
{
public:
  /**
   * @brief Constructs an ObManifest object with the specified path.
   * @param path The directory path where the manifest files are stored.
   */
  ObManifest(const std::string &path) : path_(filesystem::path(path)) { current_path_ = path_ / "CURRENT"; }

  /**
   * @brief Opens the manifest file and initializes the reader and writer.
   * @return RC::SUCCESS if the file is opened successfully, otherwise RC::IOERR_OPEN.
   */
  RC open();

  /**
   * @brief Pushes a record or snapshot to the writer.
   *
   * This function serializes either an ObManifestCompaction ,ObManifestNewMemtable or ObManifestSnapshot to JSON format
   * and writes it to the file. It also prefixes the JSON data with its length for easy retrieval.
   *
   * @tparam T The type of the data to push (either ObManifestCompaction ,ObManifestNewMemtable or ObManifestSnapshot).
   * @param data The data to be serialized and pushed to the file.
   * @return RC::SUCCESS if Push successful
   */
  template <typename T>
  RC push(const T &data)
  {
    static_assert(std::is_same<T, ObManifestCompaction>::value || std::is_same<T, ObManifestSnapshot>::value ||
                      std::is_same<T, ObManifestNewMemtable>::value,
        "push() only supports ObManifestCompaction ,ObManifestNewMemtable and ObManifestSnapshot types.");
    Json::Value json = JsonConverter::to_json(data);
    string      str  = json.toStyledString();
    size_t      len  = str.size();
    RC          rc   = writer_->write(string_view{reinterpret_cast<char *>(&len), sizeof(len)});
    if (OB_FAIL(rc)) {
      LOG_WARN("Failed to push record into manifest file %s, rc %s", writer_->file_name().c_str(), strrc(rc));
      return rc;
    }
    rc = writer_->write(str);
    if (OB_FAIL(rc)) {
      LOG_WARN("Failed to push record into manifest file %s,  rc %s", writer_->file_name().c_str(), strrc(rc));
      return rc;
    }
    rc = writer_->flush();
    if (OB_FAIL(rc)) {
      LOG_WARN("Failed to flush data of manifest file %s, rc %s", writer_->file_name().c_str(), strrc(rc));
      return rc;
    }
    return rc;
  }

  /**
   * @brief Redirects to a new manifest file.
   *
   * @param snapshot The snapshot containing the data to be written to the new manifest file.
   * @param memtable The mamtable record containing the memtable_id to be written to the new manifest file.
   * @return RC::SUCCESS,if ObManifest successfully writes snapshot and memtable records into a new manifest file.
   */
  RC redirect(const ObManifestSnapshot &snapshot, const ObManifestNewMemtable &memtable);

  /**
   * @brief Redirects to a new manifest file.
   *
   * @param snapshot Record a snapshot of ObLsm.
   * @param memtable Record the latest WAL's id.
   * @param compactions Record changes to each compaction operation.
   * @return RC::SUCCESS, If there is nothing wrong with the recovery process.
   */
  RC recover(std::unique_ptr<ObManifestSnapshot> &snapshot_record,
      std::unique_ptr<ObManifestNewMemtable> &memtbale_record, std::vector<ObManifestCompaction> &compactions);

  uint64_t latest_seq{0};  ///< The latest sequence number persisted in the LSM.

  friend class ObManifestTester;

private:
  /**
   * @brief Constructs the path to the manifest file.
   *
   * @param path The base path where the manifest file is stored.
   * @param mf_seq The sequence ID of the manifest file.
   * @return The full path to the manifest file.
   */
  string get_manifest_file_path(string path, uint64_t mf_seq)
  {
    return filesystem::path(path) / (std::to_string(mf_seq) + MANIFEST_SUFFIX);
  }

private:
  filesystem::path path_;          ///< The directory path where manifest files are stored.
  filesystem::path current_path_;  ///< The path to the CURRENT file that tracks the latest manifest file.
  uint64_t         mf_seq_;        ///< The sequence ID of the current manifest file.

  std::unique_ptr<ObFileWriter> writer_;  ///< The file writer for manifest data.
  std::unique_ptr<ObFileReader> reader_;  ///< The file reader for manifest data.
};

}  // namespace oceanbase
