#pragma once

#include <functional>

namespace common {

class DeferHelper
{
public: 
  DeferHelper(const std::function<void()> &defer) : defer_(defer)
  {}

  ~DeferHelper()
  {
    defer_();
  }

private:
  const std::function<void()> &defer_;
};

} // namespace common

#define DEFER(callback) common::DeferHelper defer_helper_##__LINE__(callback)
