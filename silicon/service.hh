#pragma once

#include <map>
#include <iod/foreach.hh>
#include <silicon/api.hh>
#include <silicon/error.hh>
#include <silicon/di_factories.hh>
#include <iod/di.hh>

namespace sl
{
  using namespace iod;

  template <typename M, typename S, typename... ARGS>
  struct ws_handler_base
  {
    ws_handler_base() {}
    virtual void operator()(M& middlewares, S& s,
                            ARGS&...) = 0;

  };
  
  template <typename A, typename R, typename F, typename M, typename S, typename... ARGS>
  struct ws_handler : public ws_handler_base<M, S, ARGS...>
  {
    typedef A arguments_type;
    typedef R return_type;
    typedef F function_type;
    
    ws_handler(F f) : f_(f) {}

    virtual void operator()(M& middlewares, S& s, ARGS&... args)
    {
      // Decode arguments
      arguments_type arguments;
      auto method = &S::template deserialize<arguments_type>;
      di_call_method(s, method, arguments, args...);

      // Call the procedure and serialize its result.
      typedef iod::callable_arguments_tuple_t<F> T;
      static_if<std::is_same<callable_return_type_t<F>, void>::value>(
        [&, this] (auto& arguments) { // If the procedure does not return a value just call it.
          di_factories_call(f_, middlewares, arguments, args...);
        },
        [&, this] (auto& arguments) { // If the procedure returns a value, serialize it.
          auto ret = di_factories_call(f_, middlewares, arguments, args...);
          auto method = &S::template serialize<decltype(ret)>;
          di_call_method(s, method, ret, args...);
        }, arguments);
    }
    
  private:
    F f_;
  };


  template <typename S, typename A, typename... ARGS>
  struct service
  {
    typedef typename A::middlewares_type middlewares_type;
    
    service(const A& api)
      : api_(api)
    {
      index_api(api_.procedures());
      api_.initialize_factories();
    }

    template <typename O>
    void index_api(O o, std::string prefix = "")
    {
      foreach(o) | [this, prefix] (auto& f)
      {
        static_if<is_sio<decltype(f.value())>::value>(
          [&] (auto _this, auto f) { // If sio, recursion.
            _this->index_api(f.value(),
                             prefix + f.symbol().name() + std::string("/"));
          },
          [&] (auto _this, auto f) { // Else, register the procedure.
            typedef std::remove_reference_t<decltype(f.value())> F;
            typedef typename F::arguments_type AT;
            typedef typename F::return_type R;
            typedef typename F::function_type FN;
            std::string name = std::string("/") + prefix + f.symbol().name();
            _this->routing_table_[name] =
              new ws_handler<AT, R, FN, middlewares_type, S, ARGS...>(f.value().function());
          }, this, f);
      };
    }

    auto& api() { return api_; }

    void operator()(const std::string& route,
                    ARGS... args)
    {
      auto it = routing_table_.find(route);
      if (it != routing_table_.end())
        it->second->operator()(api_.middlewares(), s_, args...);
      else
        throw error::not_found("Route ", route, " does not exist.");
    }

    A api_;
    S s_;
    std::map<std::string, ws_handler_base<middlewares_type, S, ARGS...>*> routing_table_;
  };

}
