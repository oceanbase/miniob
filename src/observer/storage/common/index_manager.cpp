/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Longda on 2021/4/13.
//
#include "storage/common/index_manager.h"
#include "rc.h"

RC createIndex(const char *fileName, AttrType attrType, int attrLength) {

  //TODO
  return RC::SUCCESS;
}

RC openIndex(const char *fileName, IndexHandle *indexHandle) {
  //TODO
  return RC::SUCCESS;
}

RC closeIndex(IndexHandle *indexHandle) {
  //TODO
  return RC::SUCCESS;
}

RC insertEntry(IndexHandle *indexHandle, void *data, const RID *rid) {
  //TODO
  return RC::SUCCESS;
}

RC deleteEntry(IndexHandle *indexHandle, void *data, const RID *rid) {
  //TODO
  return RC::SUCCESS;
}

RC openIndexScan(IndexScan *indexScan, IndexHandle *indexHandle,
                 CompOp compOp, char *value) {
  //TODO
  return RC::SUCCESS;
}

RC getNextIndexEntry(IndexScan *indexScan, RID *rid) {
  //TODO
  return RC::SUCCESS;
}

RC closeIndexScan(IndexScan *indexScan) {
  //TODO
  return RC::SUCCESS;
}

RC getIndexTree(char *fileName, Tree *index) {
  //TODO
  return RC::SUCCESS;
}
