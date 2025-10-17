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
#include "cppjieba/Jieba.hpp"
#include "common/sys/rc.h"
#include "storage/tokenizer/tokenizer.h"

class JiebaTokenizer : public Tokenizer
{
public:
  JiebaTokenizer()           = default;
  ~JiebaTokenizer() override = default;
  RC cut(std::string &text, std::vector<std::string> &tokens) override;

private:
  cppjieba::Jieba jieba;
};