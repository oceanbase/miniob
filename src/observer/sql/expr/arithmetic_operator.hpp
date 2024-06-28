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
#include "common/math/simd_util.h"
#endif

#include "storage/common/column.h"

struct Equal
{
  template <class T>
  static inline bool operation(const T &left, const T &right)
  {
    return left == right;
  }
#if defined(USE_SIMD)
  static inline __m256 operation(const __m256 &left, const __m256 &right)
  {
    return _mm256_cmp_ps(left, right, _CMP_EQ_OS);
  }

  static inline __m256i operation(const __m256i &left, const __m256i &right) { return _mm256_cmpeq_epi32(left, right); }
#endif
};
struct NotEqual
{
  template <class T>
  static inline bool operation(const T &left, const T &right)
  {
    return left != right;
  }
#if defined(USE_SIMD)
  static inline __m256 operation(const __m256 &left, const __m256 &right)
  {
    return _mm256_cmp_ps(left, right, _CMP_NEQ_OS);
  }

  static inline __m256i operation(const __m256i &left, const __m256i &right)
  {
    return _mm256_xor_si256(_mm256_set1_epi32(-1), _mm256_cmpeq_epi32(left, right));
  }
#endif
};

struct GreatThan
{
  template <class T>
  static inline bool operation(const T &left, const T &right)
  {
    return left > right;
  }
#if defined(USE_SIMD)
  static inline __m256 operation(const __m256 &left, const __m256 &right)
  {
    return _mm256_cmp_ps(left, right, _CMP_GT_OS);
  }

  static inline __m256i operation(const __m256i &left, const __m256i &right) { return _mm256_cmpgt_epi32(left, right); }
#endif
};

struct GreatEqual
{
  template <class T>
  static inline bool operation(const T &left, const T &right)
  {
    return left >= right;
  }

#if defined(USE_SIMD)
  static inline __m256 operation(const __m256 &left, const __m256 &right)
  {
    return _mm256_cmp_ps(left, right, _CMP_GE_OS);
  }

  static inline __m256i operation(const __m256i &left, const __m256i &right)
  {
    return _mm256_cmpgt_epi32(left, right) | _mm256_cmpeq_epi32(left, right);
  }
#endif
};

struct LessThan
{
  template <class T>
  static inline bool operation(const T &left, const T &right)
  {
    return left < right;
  }
#if defined(USE_SIMD)
  static inline __m256 operation(const __m256 &left, const __m256 &right)
  {
    return _mm256_cmp_ps(left, right, _CMP_LT_OS);
  }

  static inline __m256i operation(const __m256i &left, const __m256i &right) { return _mm256_cmpgt_epi32(right, left); }
#endif
};

struct LessEqual
{
  template <class T>
  static inline bool operation(const T &left, const T &right)
  {
    return left <= right;
  }
#if defined(USE_SIMD)
  static inline __m256 operation(const __m256 &left, const __m256 &right)
  {
    return _mm256_cmp_ps(left, right, _CMP_LE_OS);
  }

  static inline __m256i operation(const __m256i &left, const __m256i &right)
  {
    return _mm256_or_si256(_mm256_cmpgt_epi32(right, left), _mm256_cmpeq_epi32(left, right));
  }
#endif
};

struct AddOperator
{
  template <class T>
  static inline T operation(T left, T right)
  {
    return left + right;
  }

#if defined(USE_SIMD)
  static inline __m256 operation(__m256 left, __m256 right) { return _mm256_add_ps(left, right); }

  static inline __m256i operation(__m256i left, __m256i right) { return _mm256_add_epi32(left, right); }
#endif
};

struct SubtractOperator
{
  template <class T>
  static inline T operation(T left, T right)
  {
    return left - right;
  }
  // your code here
#if defined(USE_SIMD)
  static inline __m256 operation(__m256 left, __m256 right) { exit(-1); }

  static inline __m256i operation(__m256i left, __m256i right) { exit(-1); }
#endif
};

struct MultiplyOperator
{
  template <class T>
  static inline T operation(T left, T right)
  {
    return left * right;
  }
// your code here
#if defined(USE_SIMD)
  static inline __m256 operation(__m256 left, __m256 right) { exit(-1); }

  static inline __m256i operation(__m256i left, __m256i right) { exit(-1); }
#endif
};

struct DivideOperator
{
  template <class T>
  static inline T operation(T left, T right)
  {
    // TODO: `right = 0` is invalid
    return left / right;
  }

#if defined(USE_SIMD)
  static inline __m256  operation(__m256 left, __m256 right) { return _mm256_div_ps(left, right); }
  static inline __m256i operation(__m256i left, __m256i right)
  {

    __m256 left_float   = _mm256_cvtepi32_ps(left);
    __m256 right_float  = _mm256_cvtepi32_ps(right);
    __m256 result_float = _mm256_div_ps(left_float, right_float);
    return _mm256_cvttps_epi32(result_float);
    ;
  }
#endif
};

struct NegateOperator
{
  template <class T>
  static inline T operation(T input)
  {
    return -input;
  }
};

template <typename T, bool LEFT_CONSTANT, bool RIGHT_CONSTANT, class OP>
void compare_operation(T *left, T *right, int n, std::vector<uint8_t> &result)
{
#if defined(USE_SIMD)
  int           i          = 0;
  if constexpr (std::is_same<T, float>::value) {
    for (; i <= n - SIMD_WIDTH; i += SIMD_WIDTH) {
      __m256 left_value, right_value;

      if constexpr (LEFT_CONSTANT) {
        left_value = _mm256_set1_ps(left[0]);
      } else {
        left_value = _mm256_loadu_ps(&left[i]);
      }

      if constexpr (RIGHT_CONSTANT) {
        right_value = _mm256_set1_ps(right[0]);
      } else {
        right_value = _mm256_loadu_ps(&right[i]);
      }

      __m256  result_values = OP::operation(left_value, right_value);
      __m256i mask          = _mm256_castps_si256(result_values);

      for (int j = 0; j < SIMD_WIDTH; j++) {
        result[i + j] &= mm256_extract_epi32_var_indx(mask, j) ? 1 : 0;
      }
    }
  } else if constexpr (std::is_same<T, int>::value) {
    for (; i <= n - SIMD_WIDTH; i += SIMD_WIDTH) {
      __m256i left_value, right_value;

      if (LEFT_CONSTANT) {
        left_value = _mm256_set1_epi32(left[0]);
      } else {
        left_value = _mm256_loadu_si256((__m256i *)&left[i]);
      }

      if (RIGHT_CONSTANT) {
        right_value = _mm256_set1_epi32(right[0]);
      } else {
        right_value = _mm256_loadu_si256((__m256i *)&right[i]);
      }

      __m256i result_values = OP::operation(left_value, right_value);

      for (int j = 0; j < SIMD_WIDTH; j++) {
        result[i + j] &= mm256_extract_epi32_var_indx(result_values, j) ? 1 : 0;
      }
    }
  }

  for (; i < n; i++) {
    auto &left_value  = left[LEFT_CONSTANT ? 0 : i];
    auto &right_value = right[RIGHT_CONSTANT ? 0 : i];
    result[i] &= OP::operation(left_value, right_value) ? 1 : 0;
  }
#else
  for (int i = 0; i < n; i++) {
    auto &left_value  = left[LEFT_CONSTANT ? 0 : i];
    auto &right_value = right[RIGHT_CONSTANT ? 0 : i];
    result[i] &= OP::operation(left_value, right_value) ? 1 : 0;
  }
#endif
}

template <bool LEFT_CONSTANT, bool RIGHT_CONSTANT, typename T, class OP>
void binary_operator(T *left_data, T *right_data, T *result_data, int size)
{
#if defined(USE_SIMD)
  int i = 0;

  if constexpr (std::is_same<T, float>::value) {
    for (; i <= size - SIMD_WIDTH; i += SIMD_WIDTH) {
      __m256 left_value, right_value;

      if (LEFT_CONSTANT) {
        left_value = _mm256_set1_ps(left_data[0]);
      } else {
        left_value = _mm256_loadu_ps(&left_data[i]);
      }

      if (RIGHT_CONSTANT) {
        right_value = _mm256_set1_ps(right_data[0]);
      } else {
        right_value = _mm256_loadu_ps(&right_data[i]);
      }

      __m256 result_value = OP::operation(left_value, right_value);
      _mm256_storeu_ps(&result_data[i], result_value);
    }
  } else if constexpr (std::is_same<T, int>::value) {
    for (; i <= size - SIMD_WIDTH; i += SIMD_WIDTH) {
      __m256i left_value, right_value;

      if (LEFT_CONSTANT) {
        left_value = _mm256_set1_epi32(left_data[0]);
      } else {
        left_value = _mm256_loadu_si256((const __m256i *)&left_data[i]);
      }

      if (RIGHT_CONSTANT) {
        right_value = _mm256_set1_epi32(right_data[0]);
      } else {
        right_value = _mm256_loadu_si256((const __m256i *)&right_data[i]);
      }

      __m256i result_value = OP::operation(left_value, right_value);
      _mm256_storeu_si256((__m256i *)&result_data[i], result_value);
    }
  }

  // 处理剩余未对齐的数据
  for (; i < size; i++) {
    auto &left_value  = left_data[LEFT_CONSTANT ? 0 : i];
    auto &right_value = right_data[RIGHT_CONSTANT ? 0 : i];
    result_data[i]    = OP::template operation<T>(left_value, right_value);
  }
#else
  for (int i = 0; i < size; i++) {
    auto &left_value  = left_data[LEFT_CONSTANT ? 0 : i];
    auto &right_value = right_data[RIGHT_CONSTANT ? 0 : i];
    result_data[i]    = OP::template operation<T>(left_value, right_value);
  }
#endif
}

template <bool CONSTANT, typename T, class OP>
void unary_operator(T *input, T *result_data, int size)
{
  for (int i = 0; i < size; i++) {
    auto &value    = input[CONSTANT ? 0 : i];
    result_data[i] = OP::template operation<T>(value);
  }
}

// TODO: optimized with simd
template <typename T, bool LEFT_CONSTANT, bool RIGHT_CONSTANT>
void compare_result(T *left, T *right, int n, std::vector<uint8_t> &result, CompOp op)
{
  switch (op) {
    case CompOp::EQUAL_TO: {
      compare_operation<T, LEFT_CONSTANT, RIGHT_CONSTANT, Equal>(left, right, n, result);
      break;
    }
    case CompOp::NOT_EQUAL: {
      compare_operation<T, LEFT_CONSTANT, RIGHT_CONSTANT, NotEqual>(left, right, n, result);
      break;
    }
    case CompOp::GREAT_EQUAL: {
      compare_operation<T, LEFT_CONSTANT, RIGHT_CONSTANT, GreatEqual>(left, right, n, result);
      break;
    }
    case CompOp::GREAT_THAN: {
      compare_operation<T, LEFT_CONSTANT, RIGHT_CONSTANT, GreatThan>(left, right, n, result);
      break;
    }
    case CompOp::LESS_EQUAL: {
      compare_operation<T, LEFT_CONSTANT, RIGHT_CONSTANT, LessEqual>(left, right, n, result);
      break;
    }
    case CompOp::LESS_THAN: {
      compare_operation<T, LEFT_CONSTANT, RIGHT_CONSTANT, LessThan>(left, right, n, result);
      break;
    }
    default: break;
  }
}