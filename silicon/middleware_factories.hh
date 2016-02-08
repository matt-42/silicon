#pragma once

#include <tuple>

namespace sl
{
  template <typename... F>
  auto middleware_factories(F... factories)
  {
    return std::make_tuple(factories...);
  }
}
