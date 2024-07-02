/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <type_traits>

#include "sql/expr/aggregate_state.h"

#ifdef USE_SIMD
#include "common/math/simd_util.h"
#endif
template <typename T>
void SumState<T>::update(const T *values, int size)
{
#ifdef USE_SIMD
  if constexpr (std::is_same<T, float>::value) {
    value += mm256_sum_ps(values, size);
  } else if constexpr (std::is_same<T, int>::value) {
    value += mm256_sum_epi32(values, size);
  }
#else
  for (int i = 0; i < size; ++i) {
 	  value += values[i];
  }
#endif
}

template class SumState<int>;
template class SumState<float>;