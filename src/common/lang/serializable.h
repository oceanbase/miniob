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
// Created by Longda on 2010
//

#pragma once

#include "common/lang/string.h"
#include "common/lang/iostream.h"
namespace common {

/**
 * Through this type to determine object type
 */
enum
{
  MESSAGE_BASIC          = 100,
  MESSAGE_BASIC_REQUEST  = 1000,
  MESSAGE_BASIC_RESPONSE = -1000
};

class Deserializable
{
public:
  /*
   * deserialize buffer to one object
   * @param[in]buffer,     buffer to store the object serialized bytes
   * @return *             object
   */
  virtual void *deserialize(const char *buffer, int bufLen) = 0;
};

class Serializable
{
public:
  /*
   * serialize this object to bytes
   * @param[in] buffer,    buffer to store the object serialized bytes,
   *                       please make sure the buffer is enough
   * @param[in] bufferLen, buffer length
   * @return,              used buffer length -- success, -1 means failed
   */
  virtual int serialize(ostream &os) const = 0;

  /*
   * deserialize bytes to this object
   * @param[in] buffer      buffer to store the object serialized bytes
   * @param[in] bufferLen   buffer lenght
   * @return                used buffer length -- success , -1 --failed
   */
  virtual int deserialize(istream &is) = 0;

  /**
   * get serialize size
   * @return                >0 -- success, -1 --failed
   */
  virtual int get_serial_size() const = 0;

  /**
   * this function will generalize one output string
   */
  virtual void to_string(string &output) const = 0;
};

}  // namespace common
