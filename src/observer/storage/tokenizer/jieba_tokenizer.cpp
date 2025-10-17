/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/tokenizer/jieba_tokenizer.h"
#include "common/utils/private_accessor.h"

IMPLEMENT_GET_PRIVATE_VAR(KeywordExtractor, cppjieba::KeywordExtractor, stopWords_, std::unordered_set<std::string>)

RC JiebaTokenizer::cut(std::string &text, std::vector<std::string> &tokens)
{
    // Implement Jieba tokenizer logic here
    jieba.Cut(text, tokens);
    // Remove stop words
    auto &stopWords_ = *GET_PRIVATE(cppjieba::KeywordExtractor, &jieba.extractor, KeywordExtractor, stopWords_);
    tokens.erase(std::remove_if(tokens.begin(),
                        tokens.end(),
                        [&stopWords_](const std::string &word) { return stopWords_.find(word) != stopWords_.end(); }),
        tokens.end());
    return RC::SUCCESS;
}