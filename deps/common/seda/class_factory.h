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
// Created by Longda on 2010
//

#ifndef __COMMON_SEDA_CLASS_FACTORY_H__
#define __COMMON_SEDA_CLASS_FACTORY_H__

#include <list>

#include "common/defs.h"
#include "common/log/log.h"
namespace common {

/**
 *  A class to construct arbitrary subclass instances
 *
 *  This class provides a general solution to constructing instances
 *  of subclasses from a common generic base.  The template is
 *  instantiated for each generic base class.  Then each new subclass
 *  class creates a corresponding ClassFactory instance which includes
 *  a tag which identifies the class and a factory function which
 *  constructs an instance of the class when invoked.  The factory
 *  function takes the tag and a properties object as parameters.
 *
 *  Each class factory instance MUST be constructed before the first
 *  call to make_instance().  Otherwise, the entry will not be found
 *  on the list.  To ensure this, if the class factory instance is
 *  provided by a library, it should be declared with static linkage
 *  within the library initialization routine.  If the class factory
 *  instance is provided by the main application, it should be declared
 *  with static linkage in a global initialization routine.
 */

template <class T>
class ClassFactory {

public:
  typedef T *(*FactoryFunc)(const std::string &);

  /**
   * Constructor
   * @param[in] tag     Tag identifies a particular sub-class
   * @param[in] func    Factory function to create sub-class instance
   *
   * @post (tag,func) pair is entered into global factory list
   */
  ClassFactory(const std::string &tag, FactoryFunc func);

  // Destructor
  ~ClassFactory();

  /**
   * Construct an instance of a specified sub-class
   * @param[in] tag     Identifies sub-class to instantiate
   * @param[in] prop    Properties desired for the instance
   *
   * @return a reference to the desired instance
   */
  static T *make_instance(const std::string &tag);

private:
  // Accessor function that gets the head of the factory list
  static ClassFactory<T> *&fact_list_head();

  std::string identifier_;  // identifier for this factory
  FactoryFunc fact_func_;   // factory function for this class
  ClassFactory<T> *next_;   // next factory in global list
};

/**
 * Accessor function that gets the head of the factory list
 * Implementation notes:
 * The head pointer in the list must be initialized to NULL before
 * it is accessed from any ClassFactory<T> constructor.  We cannot
 * rely on C++ constructor order to achieve this.  Instead, we wrap
 * the list head in an accessor function, and declare its linkage
 * as static.  C++ guarantees that the first time the function is
 * invoked (from anywhere) the static local will be initialized.
 */
template <class T>
ClassFactory<T> *&ClassFactory<T>::fact_list_head()
{
  static ClassFactory<T> *fact_list = NULL;
  return fact_list;
}

/**
 * Constructor
 * Implementation notes:
 * constructor places current instance on the global factory list.
 */
template <class T>
ClassFactory<T>::ClassFactory(const std::string &tag, FactoryFunc func) : identifier_(tag), fact_func_(func)
{
  next_ = fact_list_head();
  fact_list_head() = this;
}

// Destructor
template <class T>
ClassFactory<T>::~ClassFactory()
{}

/**
 * Construct an instance of a specified sub-class
 * Implementation notes:
 * scan global list to find matching tag and use the factory func to
 * create an instance.
 */
template <class T>
T *ClassFactory<T>::make_instance(const std::string &tag)
{
  T *instance = NULL;
  ClassFactory<T> *current = fact_list_head();

  // search the global factory list for a match
  while ((current != NULL) && (tag != current->identifier_)) {
    current = current->next_;
  }

  // if we have a match, create and return an instance
  if (current != NULL) {
    FactoryFunc fptr = current->fact_func_;
    instance = (*fptr)(tag);
  }

  ASSERT((instance != NULL), "%s%s", tag.c_str(), "instance not created");
  return instance;
}

}  // namespace common
#endif  // __COMMON_SEDA_CLASS_FACTORY_H__
