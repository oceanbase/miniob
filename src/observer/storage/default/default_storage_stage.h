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

#ifndef __OBSERVER_STORAGE_DEFAULT_STORAGE_STAGE_H__
#define __OBSERVER_STORAGE_DEFAULT_STORAGE_STAGE_H__

#include "common/seda/stage.h"
#include "common/metrics/metrics.h"

class DefaultHandler;

class DefaultStorageStage : public common::Stage {
public:
  ~DefaultStorageStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  DefaultStorageStage(const char *tag);
  bool set_properties() override;

  bool initialize() override;
  void cleanup() override;
  void handle_event(common::StageEvent *event) override;
  void callback_event(common::StageEvent *event,
                     common::CallbackContext *context) override;

private:
  std::string load_data(const char *db_name, const char *table_name, const char *file_name);

protected:
  common::SimpleTimer *query_metric_ = nullptr;
  static const std::string QUERY_METRIC_TAG;

private:
  DefaultHandler * handler_;
};

#endif //__OBSERVER_STORAGE_DEFAULT_STORAGE_STAGE_H__
