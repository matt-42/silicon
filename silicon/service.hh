#pragma once

#include <map>
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
    
    ws_handler(P proc) : procedure_(proc), f_(proc.function()) {}

    virtual void operator()(M& middlewares, S& s, ARGS&... args)
    {
      // Decode arguments
      arguments_type arguments = procedure_.default_arguments();
      auto method = &S::template deserialize<P, arguments_type>;
      di_call_method(s, method, arguments, procedure_, args...);

      // Call the procedure and serialize its result.
      static_if<std::is_same<return_type, void>::value>(
        [&, this] (auto& arguments) { // If the procedure does not return a value just call it.
          di_factories_call(f_, middlewares, arguments, args...);
          auto method = &S::template serialize<const std::string>;
          di_call_method(s, method, std::string(""), args...);
        },
        [&, this] (auto& arguments) { // If the procedure returns a value, serialize it.
          auto ret = di_factories_call(f_, middlewares, arguments, args...);
          auto method = &S::template serialize<decltype(ret)>;
          di_call_method(s, method, ret, args...);
        }, arguments);
    }
    
  private:
    P procedure_;
    function_type f_;
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
      std::cout << "index_api_start." << std::endl;
      foreach(o) | [this, prefix] (auto& f)
      {
        static_if<is_tuple<decltype(f.content)>::value>(
          [&] (auto _this, auto f) { // If sio, recursion.
            _this->index_api(f.content,
                             prefix + f.route.path_as_string(false));
          },
          [&] (auto _this, auto f) { // Else, register the procedure.
            typedef std::remove_reference_t<decltype(f.content)> P;
            std::string name = f.route.verb_as_string() + prefix + f.route.path_as_string(false);
            std::cout << "index_api: " << name << std::endl;
            _this->routing_table_[name] =
              new ws_handler<P, middlewares_type, S, ARGS...>(f.content);
          }, this, f);
      };
      std::cout << "index_api_end." << std::endl;
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
    //std::map<std::string, ws_handler_base<middlewares_type, S, ARGS...>*> routing_table_;
    dynamic_routing_table<std::string, ws_handler_base<middlewares_type, S, ARGS...>*> routing_table_;
  };

}
