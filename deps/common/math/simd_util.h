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

#if defined(USE_SIMD)
#include <immintrin.h>

static constexpr int SIMD_WIDTH = 8;  // AVX2 (256bit)

/// @brief 从 vec 中提取下标为 i 的 int 类型的值。
int mm256_extract_epi32_var_indx(const __m256i vec, const unsigned int i);

/// @brief 数组求和
int   mm256_sum_epi32(const int *values, int size);
float mm256_sum_ps(const float *values, int size);

/// @brief selective load 的标量实现
template <typename V>
void selective_load(V *memory, int offset, V *vec, __m256i &inv);
#endif