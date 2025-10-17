/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

/**
 * @brief Non-intrusive utility to access private members of C++ classes
 *
 * This header implements a technique to safely access private member variables
 * of a class without modifying its definition — ideal for working with third-party
 * libraries where source modification is not allowed.
 *
 * Design Principle:
 * According to the C++ standard, explicit instantiation of templates ignores
 * access control (i.e., it bypasses `private`, `protected`). We leverage this
 * rule by combining friend functions and member pointers to legally obtain
 * the address of a private member.
 *
 * Reference:
 * https://zhuanlan.zhihu.com/p/693706299
 */

#pragma once

// ==========================
// Core Template Mechanism for Non-Intrusive Access
// ==========================

/**
 * @brief Generic template to enable access to private class members.
 *
 * This empty template struct serves as the foundation of the technique.
 * Its purpose is to:
 *   - Accept an `Accessor` type (user-defined helper)
 *   - Accept a class member pointer (`Member`)
 *   - Use a friend function inside to return that member pointer
 *
 * Key Insight:
 * - The C++ standard allows explicit instantiation of templates to ignore access control.
 *   ([temp.explicit]/3): "The usual access checking rules do not apply..."
 * - By explicitly instantiating this template with a private member pointer,
 *   we can legally capture its address.
 *
 * Example:
 *   template struct AccessPrivate<MyAccessor, &MyClass::m_data>;
 *   This line will successfully capture the private member `m_data`.
 */
template <typename Accessor, typename Accessor::Member Member>
struct AccessPrivate
{
  /**
   * @brief Friend function that returns the private member pointer.
   *
   * Declared as a friend of `Accessor`, this function becomes visible
   * when the template is explicitly instantiated. It simply returns
   * the `Member` pointer passed as a non-type template argument.
   */
  friend auto GetPrivate(Accessor) -> decltype(Member) { return Member; }
};

/**
 * @brief Macro: Declare access capability for a private member variable.
 *
 * This macro generates a unique accessor structure and triggers explicit
 * instantiation of `AccessPrivate`, enabling access to the specified private member.
 *
 * @param FlattenName A flat identifier (must not contain ::), used to generate a unique name
 *                    e.g., "KeywordExtractor"
 * @param InClass     Full class name, possibly qualified with namespaces
 *                    e.g., cppjieba::KeywordExtractor
 * @param VarName     Name of the private member variable (without quotes)
 *                    e.g., stopWords_
 * @param VarType     Type of the member variable (without *, &, or const)
 *                    e.g., std::unordered_set<std::string>
 *
 * @note This macro should be used only once per (class, member) pair,
 *       preferably in a .cpp file to avoid ODR (One Definition Rule) violations.
 * @note The generated struct resides in the global namespace, named like:
 *       __privacc_KeywordExtractor_stopWords_
 */
#define IMPLEMENT_GET_PRIVATE_VAR(FlattenName, InClass, VarName, VarType) \
  struct __privacc_##FlattenName##_##VarName                              \
  {                                                                       \
    /* Define the member pointer type: e.g., T InClass::* */              \
    using Member = VarType InClass::*;                                    \
    /* Declare friend function to return the member pointer */            \
    friend Member GetPrivate(__privacc_##FlattenName##_##VarName);        \
  };                                                                      \
  /* Explicitly instantiate AccessPrivate to capture &InClass::VarName */ \
  template struct AccessPrivate<__privacc_##FlattenName##_##VarName, &InClass::VarName>;

/**
 * @brief Macro: Obtain a pointer to the private member of an object instance.
 *
 * Returns a mutable pointer (`T*`) to the private member of the given object.
 *
 * @param InClass     Full class name (same as in IMPLEMENT_GET_PRIVATE_VAR)
 * @param InObjPtr    Pointer to the object instance (type: InClass*)
 * @param FlattenName Same flat identifier used in IMPLEMENT_GET_PRIVATE_VAR
 * @param VarName     Name of the private member variable
 *
 * @return T* - Pointer to the private member, allowing both read and write access.
 *
 * @note Uses `const_cast` internally to remove constness — use carefully.
 * @note Evaluation steps:
 *       1. Calls `GetPrivate(Accessor{})` → returns member pointer (e.g., T MyClass::*)
 *       2. Applies .* operator on the object to get the actual member
 *       3. Takes address to yield `T*`
 */
#define GET_PRIVATE(InClass, InObjPtr, FlattenName, VarName) \
  (&((*const_cast<InClass *>((InObjPtr))).*GetPrivate(__privacc_##FlattenName##_##VarName())))

/**
 * @example Usage Example
 *
 * Suppose you have a third-party class defined as:
 *
 *   namespace cppjieba {
 *   class KeywordExtractor {
 *   private:
 *       std::unordered_set<std::string> stopWords_;
 *   };
 *   }
 *
 * In your .cpp file:
 *
 *   // Declare access once (do NOT put in header)
 *   IMPLEMENT_GET_PRIVATE_VAR(KeywordExtractor, cppjieba::KeywordExtractor, stopWords_,
 * std::unordered_set<std::string>)
 *
 *   void example() {
 *       cppjieba::KeywordExtractor extractor;
 *
 *       // Get pointer to private member
 *       auto* pStopWords = GET_PRIVATE(cppjieba::KeywordExtractor, &extractor, KeywordExtractor, stopWords_);
 *
 *       // Now you can modify it freely
 *       pStopWords->insert("hello");
 *   }
 *
 * @warning
 *   - Do NOT place `IMPLEMENT_GET_PRIVATE_VAR` in header files — it will cause multiple definition errors.
 *   - Choose unique `FlattenName`s to prevent naming collisions.
 *   - This is an advanced metaprogramming trick. While not undefined behavior,
 *     it should only be used when necessary (e.g., testing, legacy integration).
 *   - Works only for data members with known types and names.
 */