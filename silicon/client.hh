#include <memory>
#include <sstream>
#include <map>
#include <mimosa/http/client-channel.hh>
#include <iod/json.hh>
#include <silicon/mimosa_backend.hh>
#include <silicon/symbols.hh>
#include <silicon/server.hh>

namespace iod
{

  struct http_client
  {
    http_client()
    {
    }

    bool connect(const std::string& host, int port, bool ssl)
    {
      return mimosa_client.connect(host, port, ssl);
    }

    template <typename R, typename S, typename A>
    auto remote_call(S symbol, const A& args)
    {
      using namespace mimosa;

      // Generate url.
      std::stringstream ss;
      ss << "http://0.0.0.0:12345/" << symbol.name();
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

      // Decode result.
      R result;
      std::string body;
      char buf[1024];
      int n;
      while ((n = rr->read(buf, 1024))) { body.append(buf, n); }
      
      if (rr->status() == 200)
      {
        json_decode(result, body);
      }
      else
        std::cout << body << std::endl;

      // Decode cookies.
      auto new_cookies = rr->unparsedHeaders().equal_range("Set-Cookie");
      for (auto it = new_cookies.first; it != new_cookies.second; it++)
      {
        auto c = it->second;
        char* key = &c[0];
        char* key_end;
        while (std::isspace(*key) and *key) key++;
        key_end = key;
        while (std::isalnum(*key_end) and *key_end) key_end++;

        char* value = key_end;
        while (*value != '=' and *value) value++;
        while (std::isspace(*value) and *value) value++;
        char* value_end = value;
        while (std::isalnum(*value_end) and *value_end) value_end++;
        
        cookies[std::string(key, key_end)] = std::string(value, value_end);
      }

      return D(
        s::_Status = rr->status(),
        s::_Response = result);
    }

    std::map<std::string, std::string> cookies;
    mimosa::http::ClientChannel mimosa_client;
  };

  struct D_caller
  {
    template <typename... X>
    auto operator() (X&&... t) const { return D(t...); }
  };

  template <typename C, typename S, typename R>
  auto create_client_call(C c, S symbol, sio<>, R)
  {
    return [c, symbol] ()
    {
      return c->template remote_call<R>(symbol, D());
    };
  }
  
  template <typename C, typename S, typename R, typename... T>
  auto create_client_call(C c, S symbol, sio<T...> args, R)
  {
    return [c, args, symbol] (std::remove_reference_t<decltype(std::declval<T>().value())>... args2)
    {
      auto o = foreach(args.symbols_as_tuple(),
                       std::make_tuple(args2...)) | [] (auto& s, auto& v) {
        return s = v;
      };

      auto o2 = apply(o, D_caller());
      return c->template remote_call<R>(symbol, o2);
    };
  }

  template <typename A>
  auto silicon_client(const A& api, std::string host, int port)
  {
    std::shared_ptr<http_client> c(new http_client());
    if (!c->connect(host, port, false))
      throw std::runtime_error("Cannot connect to the server");

    auto calls = foreach(api) | [c] (auto m) {
      typedef std::remove_reference_t<decltype(m.value())> V;
      typedef typename V::function_type F;
      typename V::arguments_type arguments;
      return m.symbol() = create_client_call(c, m.symbol(), arguments, callable_return_type_t<F>());
    };

    return calls;
  }
}
