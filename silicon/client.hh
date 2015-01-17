#include <memory>
#include <sstream>
#include <map>
#include <mimosa/http/client-channel.hh>
#include <iod/json.hh>
//#include <silicon/mimosa_backend.hh>
#include <silicon/symbols.hh>

namespace sl
{
  using namespace iod;

  template <typename R>
  struct response_parser
  {
    static auto run(mimosa::http::ResponseReader::Ptr rr)
    {
      return D(s::_Status = rr->status());
    }
  };

  template <typename... T>
  struct response_parser<sio<T...>>
  {
    static auto run(mimosa::http::ResponseReader::Ptr rr)
    {
      sio<T...> result;
      std::string body;
      char buf[1024];
      int n;
      while ((n = rr->read(buf, 1024))) { body.append(buf, n); }

      std::string error_message;
      if (rr->status() == 200)
        json_decode(result, body);
      else
        error_message = body;

      return D(
        s::_Status = rr->status(),
        s::_Response = result,
        s::_Error = error_message);
    }
  };

  struct http_client
  {
    http_client()
    {
    }

    bool connect(const std::string& host, int port, bool ssl)
    {
      host_ = host;
      port_ = port;
      return mimosa_client.connect(host, port, ssl);
    }

    template <typename R, typename S, typename A>
    auto remote_call(S symbol, const A& args)
    {
      using namespace mimosa;

      // Generate url.
      std::stringstream ss;
      ss << "http://" << host_ << ":" << port_ << "/" << symbol;
      uri::Url url(ss.str());

      // Send request.
      http::RequestWriter rw(mimosa_client);
      rw.setUrl(url);
      rw.setMethod(mimosa::http::kMethodPost);
      rw.setProto(1, 1);
      std::string rq_body = json_encode(args);

      rw.setContentLength(rq_body.size());
      for (auto c : cookies)
        rw.addCookie(c.first, c.second);

      if (!rw.sendRequest())
        throw std::runtime_error("Error when sending request.");
        
      rw.loopWrite((const char *)&rq_body[0], rq_body.size());
      
      auto rr = rw.response();

      if (!rr)
        throw std::runtime_error("Error when sending request.");

      // Decode cookies.
      auto new_cookies = rr->unparsedHeaders().equal_range("Set-Cookie");
      for (auto it = new_cookies.first; it != new_cookies.second; it++)
      {
        auto c = it->second;
        char* key = &c[0];
        char* key_end;
        while (std::isspace(*key) and *key) key++;
        key_end = key;
        while (*key_end != '=' and *key_end) key_end++;

        char* value = key_end + 1;
        char* value_end = value;
        while (*value_end != ';' and *value_end) value_end++;

        cookies[std::string(key, key_end)] = std::string(value, value_end);
      }
      
      // Decode result.
      return response_parser<R>::run(rr);
    }

    std::map<std::string, std::string> cookies;
    mimosa::http::ClientChannel mimosa_client;
    std::string host_;
    int port_;
  };

  struct D_caller
  {
    template <typename... X>
    auto operator() (X... t) const { return D(t...); }
  };

  template <typename R, typename C, typename S>
  auto create_client_call(C c, S symbol, sio<>)
  {
    return [c, symbol] ()
    {
      return c->template remote_call<R>(symbol, D());
    };
  }
  
  template <typename R, typename C, typename S, typename... T>
  auto create_client_call(C c, S symbol, sio<T...> args)
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
  auto generate_client_methods(C& c, A api, std::string prefix = "")
  {
    return foreach(api) | [c, prefix] (auto m) {

      return static_if<is_sio<decltype(m.value())>::value>(
        [c, prefix] (auto m) { // If sio, recursion.
          return m.symbol() = generate_client_methods(c, m.value(), prefix + m.symbol().name() + "_");
        },
        [c, prefix] (auto m) { // Else, register the procedure.
          typedef std::remove_reference_t<decltype(m.value())> V;
          typedef typename V::function_type F;
          typename V::arguments_type arguments;
          
          return m.symbol() = create_client_call<callable_return_type_t<F>>
            (c, prefix + m.symbol().name(), arguments);
            }, m);

    };
  }

  template <typename A>
  auto json_client(const A& api, std::string host, int port)
  {
    std::shared_ptr<http_client> c(new http_client());
    if (!c->connect(host, port, false))
      throw std::runtime_error("Cannot connect to the server");

    return generate_client_methods(c, api.procedures());
  }
}
