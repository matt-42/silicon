#pragma once
#include <map>
#include <iod/foreach.hh>
#include <silicon/api.hh>
#include <silicon/error.hh>
#include <iod/di.hh>


namespace sl
{
  using namespace iod;

  template <typename M, typename S, typename... ARGS>
  struct ws_handler_base
  {
    typedef typename S::request_type request_type;
    typedef typename S::response_type response_type;
    
    ws_handler_base() {}
    virtual void operator()(M& middlewares, S& s,
                            request_type& request, response_type& response,
                            ARGS&...) = 0;

  };
  
  template <typename A, typename R, typename F, typename M, typename S, typename... ARGS>
  struct ws_handler : public ws_handler_base<M, S, ARGS...>
  {
    typedef A arguments_type;
    typedef R return_type;
    typedef F function_type;
    
    typedef typename S::request_type request_type;
    typedef typename S::response_type response_type;
    
    ws_handler(F f) : f_(f) {}

    template <typename... M2, typename... A2>
    auto unroll_middlewares_and_call(F f_, std::tuple<M2...> middlewares, A2&&... args)
    {
      return di_call(f_, tuple_get_by_type<M2>(middlewares)..., args...);
    }

    virtual void operator()(M& middlewares, S& s,
                            request_type& request, response_type& response, ARGS&... args)
    {
      // Decode arguments
      arguments_type arguments;
      s.deserialize(request, arguments);

      // Call the procedure and serialize its result.
      typedef iod::callable_arguments_tuple_t<F> T;
      static_if<std::is_same<callable_return_type_t<F>, void>::value>(
        [&, this] (auto& response) {
          this->unroll_middlewares_and_call(f_, middlewares, &request, &response, arguments, args...);
        },
        [&, this] (auto& response) {
          s.serialize(response, this->unroll_middlewares_and_call(f_, middlewares, &request, &response, arguments, args...));
        }, response);
    }
    
  private:
    F f_;
  };


  template <typename S, typename A, typename... ARGS>
  struct ws_service
  {
    typedef typename S::request_type request_type;
    typedef typename S::response_type response_type;
    typedef typename A::middlewares_type middlewares_type;
    
    ws_service(const A& api)
      : api_(api)
    {
      index_api(api_.procedures());
      api_.initialize_middlewares();
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
    
    void operator()(const std::string& route,
                    request_type& request,
                    response_type& response,
                    ARGS&... args)
    {
      auto it = routing_table_.find(route);
      if (it != routing_table_.end())
        it->second->operator()(api_.middlewares(), s_, request, response, args...);
      else
        throw error::not_found("Route ", route, " does not exist.");
    }

    A api_;
    S s_;
    std::map<std::string, ws_handler_base<middlewares_type, S, ARGS...>*> routing_table_;
  };

}
