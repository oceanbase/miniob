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
// Created by Assistant on 2026/05/07
// Test cases for DROP TABLE and TEXT type functionality
//

#include <string>
#include <vector>

#include "common/type/attr_type.h"
#include "common/value.h"
#include "gtest/gtest.h"

using namespace std;
using namespace common;

// Test TEXT type value operations
TEST(TextTypeTest, BasicTextValueTest)
{
  // Test TEXT type value creation
  Value  text_value;
  string test_text = "This is a test TEXT content.";
  text_value.set_string(test_text.c_str(), static_cast<int>(test_text.size()), AttrType::TEXTS);

  ASSERT_EQ(text_value.attr_type(), AttrType::TEXTS);
  ASSERT_STREQ(text_value.data(), test_text.c_str());
  ASSERT_EQ(text_value.length(), static_cast<int>(test_text.size()));
}

// Test TEXT type with different content lengths
TEST(TextTypeTest, TextTypeLengthTest)
{
  vector<string> test_texts = {
      "Short text",
      "This is a medium length text that should still fit within the allocated space.",
      string(1000, 'A'),  // Long text with 1000 'A' characters
      string(2000, 'B'),  // Very long text with 2000 'B' characters
  };

  for (const auto &test_text : test_texts) {
    Value text_value;
    text_value.set_string(test_text.c_str(), static_cast<int>(test_text.size()), AttrType::TEXTS);

    ASSERT_EQ(text_value.attr_type(), AttrType::TEXTS);
    ASSERT_EQ(text_value.length(), static_cast<int>(test_text.size()));
    ASSERT_STREQ(text_value.data(), test_text.c_str());
  }
}

// Test TEXT type with empty string
TEST(TextTypeTest, TextTypeEmptyTest)
{
  Value  empty_text;
  string empty_str = "";
  empty_text.set_string(empty_str.c_str(), static_cast<int>(empty_str.size()), AttrType::TEXTS);

  ASSERT_EQ(empty_text.attr_type(), AttrType::TEXTS);
  ASSERT_EQ(empty_text.length(), 0);
  ASSERT_STREQ(empty_text.data(), "");
}

// Test TEXT type comparison
TEST(TextTypeTest, TextTypeComparisonTest)
{
  Value text1, text2, text3;

  text1.set_string("apple", 5, AttrType::TEXTS);
  text2.set_string("banana", 6, AttrType::TEXTS);
  text3.set_string("apple", 5, AttrType::TEXTS);

  // Test comparison
  ASSERT_LT(text1.compare(text2), 0);  // "apple" < "banana"
  ASSERT_GT(text2.compare(text1), 0);  // "banana" > "apple"
  ASSERT_EQ(text1.compare(text3), 0);  // "apple" == "apple"
}

// Test TEXT type copy operations
TEST(TextTypeTest, TextTypeCopyTest)
{
  Value  original;
  string test_text = "Original TEXT content";
  original.set_string(test_text.c_str(), static_cast<int>(test_text.size()), AttrType::TEXTS);

  // Test copy constructor
  Value copy = original;
  ASSERT_EQ(copy.attr_type(), AttrType::TEXTS);
  ASSERT_STREQ(copy.data(), test_text.c_str());
  ASSERT_EQ(copy.length(), static_cast<int>(test_text.size()));

  // Test assignment
  Value assigned;
  assigned = original;
  ASSERT_EQ(assigned.attr_type(), AttrType::TEXTS);
  ASSERT_STREQ(assigned.data(), test_text.c_str());
  ASSERT_EQ(assigned.length(), static_cast<int>(test_text.size()));
}

// Test TEXT type with special characters
TEST(TextTypeTest, TextTypeSpecialCharsTest)
{
  string special_text = "Special chars: \n\t\r\"'\\中文English123!@#$%^&*()";
  Value  text_value;
  text_value.set_string(special_text.c_str(), static_cast<int>(special_text.size()), AttrType::TEXTS);

  ASSERT_EQ(text_value.attr_type(), AttrType::TEXTS);
  ASSERT_EQ(text_value.length(), static_cast<int>(special_text.size()));
  ASSERT_STREQ(text_value.data(), special_text.c_str());
}

// Test TEXT type type information
TEST(TextTypeTest, TextTypeInfoTest)
{
  Value text_value;
  text_value.set_string("test", 4, AttrType::TEXTS);

  // Test type information
  ASSERT_EQ(text_value.attr_type(), AttrType::TEXTS);
  ASSERT_STREQ(attr_type_to_string(text_value.attr_type()), "text");
}

// Test TEXT type reset functionality
TEST(TextTypeTest, TextTypeResetTest)
{
  Value text_value;
  text_value.set_string("initial content", 15, AttrType::TEXTS);

  ASSERT_EQ(text_value.attr_type(), AttrType::TEXTS);
  ASSERT_STREQ(text_value.data(), "initial content");

  // Reset the value
  text_value.reset();

  // After reset, should be undefined type
  ASSERT_EQ(text_value.attr_type(), AttrType::UNDEFINED);
}

// Test TEXT type with maximum reasonable length
TEST(TextTypeTest, TextTypeMaxLengthTest)
{
  // Test with a very long text (within TEXT field capacity)
  string long_text(4000, 'X');  // 4000 characters
  Value  text_value;
  text_value.set_string(long_text.c_str(), static_cast<int>(long_text.size()), AttrType::TEXTS);

  ASSERT_EQ(text_value.attr_type(), AttrType::TEXTS);
  ASSERT_EQ(text_value.length(), 4000);
  ASSERT_EQ(strlen(text_value.data()), 4000u);
}

// Test TEXT type in collections
TEST(TextTypeTest, TextTypeCollectionTest)
{
  vector<Value>  text_values;
  vector<string> test_strings = {"First text", "Second text with different content", "Third text"};

  for (const auto &str : test_strings) {
    Value val;
    val.set_string(str.c_str(), static_cast<int>(str.size()), AttrType::TEXTS);
    text_values.push_back(val);
  }

  ASSERT_EQ(text_values.size(), 3u);

  for (size_t i = 0; i < test_strings.size(); ++i) {
    ASSERT_EQ(text_values[i].attr_type(), AttrType::TEXTS);
    ASSERT_STREQ(text_values[i].data(), test_strings[i].c_str());
  }
}

int main(int argc, char **argv)
{
  // Initialize Google Test
  testing::InitGoogleTest(&argc, argv);

  // Run all tests
  return RUN_ALL_TESTS();
}