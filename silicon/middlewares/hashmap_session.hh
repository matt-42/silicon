#pragma once

#include <random>
#include <unordered_map>
#include <silicon/middlewares/tracking_cookie.hh>

namespace sl
{

  template <typename D>
  struct hashmap_session_factory
  {
    //typedef decltype(iod::cat(D(_created_at
    template <typename... O>
    hashmap_session_factory(O... opts)
    //: expires_(D(opts...).get(_expires, 10000))
    {
    }

    D& instantiate(tracking_cookie& token)
    {
      return sessions_[token.id()];
    }

    //int expires_;
    std::unordered_map<std::string, D> sessions_;
  };

}
