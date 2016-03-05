#pragma once

#include <mutex>
#include <cassert>
#include <memory>
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
      : mutex_(new std::mutex)
    //: expires_(D(opts...).get(_expires, 10000))
    {
    }

    D& instantiate(tracking_cookie& token)
    {
      D* res = 0;
      {
        std::unique_lock<std::mutex> l(*mutex_);
        res = &sessions_[token.id()];
      }
      assert(res != 0);
      return *res;
    }

    //int expires_;
    std::shared_ptr<std::mutex> mutex_;
    std::unordered_map<std::string, D> sessions_;
  };

}
