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

#ifndef __OBSERVER_STORAGE_MEM_STORAGE_STAGE_H__
#define __OBSERVER_STORAGE_MEM_STORAGE_STAGE_H__

#include "common/seda/stage.h"
#include "common/metrics/metrics.h"

class MemStorageStage : public common::Stage {
public:
  ~MemStorageStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  MemStorageStage(const char *tag);
  bool set_properties();

  bool initialize();
  void cleanup();
  void handle_event(common::StageEvent *event);
  void callback_event(common::StageEvent *event,
                     common::CallbackContext *context);

protected:
  common::SimpleTimer *queryMetric = nullptr;
  static const std::string QUERY_METRIC_TAG;
private:
};

#endif //__OBSERVER_STORAGE_MEM_STORAGE_STAGE_H__
