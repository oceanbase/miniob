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

#ifndef __COMMON_MATH_MD5_H__
#define __COMMON_MATH_MD5_H__
#include <stdio.h>
namespace common {

typedef unsigned char *POINTER;
typedef unsigned short int UINT2;
typedef unsigned int UINT4;

typedef struct {
  UINT4 state[4];           /* state (ABCD) */
  UINT4 count[2];           /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64]; /* input buffer */
} MD5_CTX;

#ifdef __cplusplus
extern "C" {
#endif

/** md5 for string
 *  parameters:
 *           string: the string to md5
 *           digest: store the md5 digest
 *  return: 0 for success, != 0 fail
 */
int MD5String(char *string, unsigned char digest[16]);

/** md5 for file
 *  parameters:
 *           filename: the filename whose content to md5
 *           digest: store the md5 digest
 *  return: 0 for success, != 0 fail
 */
int MD5File(char *filename, unsigned char digest[16]);

/** md5 for buffer
 *  parameters:
 *           buffer: the buffer to md5
 *           len: the buffer length
 *           digest: store the md5 digest
 *  return: 0 for success, != 0 fail
 */
int MD5Buffer(char *buffer, unsigned int len, unsigned char digest[16]);

void MD5Init(MD5_CTX *);

void MD5Update(MD5_CTX *, unsigned char *, unsigned int);

void MD5Final(unsigned char[16], MD5_CTX *);

#ifdef __cplusplus
}
#endif

}  // namespace common
#endif  //__COMMON_MATH_MD5_H__
