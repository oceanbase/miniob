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
// Created by Longda on 2021/4/13.
//

#pragma once

#include "common/seda/stage.h"
#include "common/rc.h"

/**
 * @brief 解析SQL语句，解析后的结果可以参考parse_defs.h
 * @ingroup SQLStage
 */
class ParseStage : public common::Stage 
{
public:
  virtual ~ParseStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  ParseStage(const char *tag);
  bool set_properties() override;

  bool initialize() override;
  void cleanup() override;
  void handle_event(common::StageEvent *event) override;

protected:
  RC handle_request(common::StageEvent *event);

private:
  Stage *resolve_stage_ = nullptr;  /// 解析后执行的下一个阶段，就是Resolve
};
