#pragma once

#include <sstream>

#include <iod/sio.hh>
#include <iod/json.hh>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <silicon/symbols.hh>
#include <silicon/backends/wspp_connection.hh>

namespace sl
{
  using namespace iod;

  using s::_silicon_ctx;

  typedef websocketpp::server<websocketpp::config::asio> wspp_server;

  struct ws_remote_client_ctx
  {
    
    void init(websocketpp::connection_hdl c,
              wspp_server* s)
    {
      connection = c;
      server = s;
    }

    template <typename R, typename S, typename A>
    auto remote_call(S route, const A& args)
    {
      std::stringstream ss;
      ss << ":" << route << ":" << json_encode(args);
      server->send(connection, ss.str(), websocketpp::frame::opcode::text);
    }
    
    websocketpp::connection_hdl connection;
    wspp_server* server;
  };

  template <typename A>
  struct ws_remote_client : public A
  {
    ws_remote_client(A a) : A(a) {}

    ws_remote_client<A>& operator()(wspp_connection c)
    {
      this->silicon_ctx->connection = c.hdl;
      this->silicon_ctx->server = c.server;
      return *this;
    }
  };

  
  template <typename R, typename C, typename S>
  auto create_ws_remote_client_call(C c, S symbol, sio<>)
  {
    return [c, symbol] ()
    {
      return c->template remote_call<R>(symbol, D());
    };
  }
  
  template <typename R, typename C, typename S, typename... T>
  auto create_ws_remote_client_call(C c, S symbol, sio<T...> args)
  {
    return [c, args, symbol] (std::remove_reference_t<decltype(std::declval<T>().value())>... args2)
    {
      auto o = foreach(args.symbols_as_tuple(),
                       std::forward_as_tuple(args2...)) | [] (auto& s, auto&& v) {
        return s = v;
      };
      
      auto o2 = apply(o, D_caller());
      return c->template remote_call<R>(symbol, o2);
    };
  }
  
  template <typename C, typename A>
  auto generate_ws_remote_client_methods(C& c, A api, std::string prefix = "/")
  {
    return foreach(api) | [c, prefix] (auto m) {

      return static_if<is_sio<decltype(m.value())>::value>(
        [c, prefix] (auto m) { // If sio, recursion.
          return m.symbol() = generate_ws_remote_client_methods(c, m.value(), prefix + m.symbol().name() + "/");
        },
        [c, prefix] (auto m) { // Else, register the procedure.
          typedef std::remove_reference_t<decltype(m.value())> V;
          typename V::arguments_type arguments;
          typedef void R; // Fixme if we want to handle client return values.
          return m.symbol() = create_ws_remote_client_call<R>
            (c, prefix + m.symbol().name(), arguments);
            }, m);

    };
    
  }
  
  template <typename A>
  auto make_ws_remote_client(const A& api)
  {
    std::shared_ptr<ws_remote_client_ctx> c(new ws_remote_client_ctx());
    auto accessor = D(_silicon_ctx = c);
    auto rc = iod::cat(generate_ws_remote_client_methods(c, api.procedures()), accessor);
    return ws_remote_client<decltype(rc)>(rc);
  }

  template <typename API>
  using ws_client = decltype(make_ws_remote_client(std::declval<API>()));

}
