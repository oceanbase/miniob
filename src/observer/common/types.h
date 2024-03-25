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
// Created by Wangyunlai on 2022/6/23.
//

#pragma once

/// 磁盘文件，包括存放数据的文件和索引(B+-Tree)文件，都按照页来组织
/// 每一页都有一个编号，称为PageNum
using PageNum = int32_t;

/// 数据文件中按照页来组织，每一页会存放一些行数据(row)，或称为记录(record)
/// 每一行(row/record)，都占用一个槽位(slot)，这些槽有一个编号，称为SlotNum
using SlotNum = int32_t;

/// LSN for log sequence number
using LSN = int32_t;

/// page的CRC校验和
using CheckSum = unsigned int;
