#pragma once

#include <unordered_map>
#include <map>
#include <memory>
#include <boost/utility/string_ref.hpp>
#include <iod/foreach.hh>
#include <silicon/api.hh>
#include <silicon/error.hh>
#include <silicon/di_factories.hh>
#include <silicon/optional.hh>
#include <silicon/dynamic_routing_table.hh>
#include <iod/di.hh>

namespace sl
{
  using namespace iod;
  using boost::string_ref;

  template <typename M, typename S, typename... ARGS>
  struct ws_handler_base
  {
    ws_handler_base() {}
    virtual void operator()(M& middlewares, S& s,
                            ARGS&...) = 0;

  };
  
  template <typename P, typename M, typename S, typename... ARGS>
  struct ws_handler : public ws_handler_base<M, S, ARGS...>
  {
    typedef typename P::arguments_type arguments_type;
    typedef typename P::return_type return_type;
    typedef typename P::function_type function_type;
    
    ws_handler(P proc) : procedure_(proc) {}

    virtual void operator()(M& middlewares, S& s, ARGS&... args)
    {
      auto f = procedure_.function();

      // Decode arguments
      arguments_type arguments = procedure_.default_arguments();
      auto method = &S::template deserialize<P, arguments_type>;
      di_call_method(s, method, arguments, procedure_, args...);

      // Call the procedure and serialize its result.
      
      static_if<std::is_same<return_type, void>::value>(
        [&, this] (auto& arguments, auto f) { // If the procedure does not return a value just call it.
          di_factories_call(f, middlewares, arguments, args...);
          auto method = &S::template serialize<const std::string>;
          di_call_method(s, method, std::string(""), args...);
        },
        [&, this] (auto& arguments, auto f) { // If the procedure returns a value, serialize it.
          auto ret = di_factories_call(f, middlewares, arguments, args...);
          auto method = &S::template serialize<decltype(ret)>;
          di_call_method(s, method, ret, args...);
        }, arguments, f);
    }
    
  private:
    P procedure_;
  };


  template <typename R, typename P, typename... PA, typename... GA>
  auto apply_global_middleware3(R route, P proc, std::tuple<PA...>*, std::tuple<GA...>*)
  {
    return make_procedure(0, route, [=] (PA&&... pa, const GA&... ga)
    {
      return proc.function()(pa...);
    });
  }

  template <typename GM, typename R, typename P>
  auto apply_global_middleware2(R route, P proc)
  {
    return apply_global_middleware3(route, proc,
                                        (callable_arguments_tuple_t<typename P::function_type>*)0, (GM*)0);
  }

  template <typename GM, typename P>
  auto apply_global_middlewares(P procedures)
  {
    return foreach(procedures) | [&] (auto m)
    {
      return static_if<is_tuple<decltype(m.content)>::value>(
        [&] (auto m) { // If sio, recursion.
          return make_api_node(m.route, apply_global_middlewares<GM>(m.content));
          //return m.symbol() = apply_global_middleware_rec(m.content, m_tuple);
        },
        [&] (auto m) { // Else, register the procedure.
          return make_api_node(m.route, apply_global_middleware2<GM>(m.route, m.content));
          // return m.route = make_procedure(0, m.route,
          //                                 apply_global_middleware(m.content, m_tuple));
        }, m);
    };
  }

  template <typename... T>
  struct add_global_middlewares
  {
    template <typename A>
    static auto to(A api)
    {
      return apply_global_middlewares<std::tuple<T...>>(api);
    }
  };
  
  
  template <typename S, typename M, typename... ARGS>
  struct service
  {
    typedef M middlewares_type;

    template <typename A>
    service(const A& api, const M& middlewares)
      : //api_(api),
        middlewares_(middlewares)
    {
      index_api(api);
      initialize_factories();
    }

    template <typename N>
    struct has_initialize_method
    {
      template <typename X>
      static char test(decltype(&X::initialize));

      template <typename X>
      static int test(...);

      static const int value = (sizeof(test<std::remove_reference_t<N>>(0)) == sizeof(char));
    };

    auto initialize_factories()
    {
      foreach(middlewares_) | [this] (auto& m)
      {
        typedef std::remove_reference_t<decltype(m)> X;
        static_if<has_initialize_method<X>::value>(
          [this] (auto& m) {
            typedef std::remove_reference_t<decltype(m)> X_;
            di_factories_call(bind_method(m, &X_::initialize), middlewares_);
          },
          [] (auto& m) {}, m);
      };
    }
    
    template <typename O>
    void index_api(O o)
    {
      foreach(o) | [this] (auto& f)
      {
        static_if<is_tuple<decltype(f.content)>::value>(
          [&] (auto _this, auto f) { // If sio, recursion.
            _this->index_api(f.content);
          },
          [&] (auto _this, auto f) { // Else, register the procedure.
            typedef std::remove_reference_t<decltype(f.content)> P;
            std::shared_ptr<std::string> name(new std::string(f.route.string_id()));
            routes.push_back(name);
            _this->routing_table_[string_ref(*name)] =
              new ws_handler<P, middlewares_type, S, ARGS...>(f.content);
          }, this, f);
      };
    }

    void operator()(const std::string& route,
                    ARGS... args)
    {
      // skip the last / of the url.
      string_ref route2(route);
      if (route2.size() != 0 and route2[route2.size() - 1] == '/')
        route2 = route2.substr(0, route2.size() - 1);
      
      auto it = routing_table_.find(route2);
      if (it != routing_table_.end())
        it->second->operator()(middlewares_, s_, args...);
      else
        throw error::not_found("Route ", route2, " does not exist.");
    }

    M middlewares_;
    S s_;

    std::vector<std::shared_ptr<std::string>> routes;
    dynamic_routing_table<ws_handler_base<middlewares_type, S, ARGS...>*> routing_table_;
  };

}
