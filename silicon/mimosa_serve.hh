#pragma once

#include <mimosa/cpu-foreach.hh>
#include <mimosa/http/server.hh>
#include <mimosa/http/response-writer.hh>
#include <mimosa/http/request-reader.hh>
#include <iod/json.hh>

#include <silicon/service.hh>
#include <silicon/cookie_token.hh>
#include <silicon/symbols.hh>

namespace sl
{
  using namespace iod;

  using s::_Secure;
  using s::_Http_only;
  using s::_Expires;
  using s::_Path;

  template <typename... O>
  struct mimosa_session_cookie
  {
    typedef mimosa::http::RequestReader request_type;
    typedef mimosa::http::ResponseWriter response_type;

    mimosa_session_cookie(O&&... options)
      : options_(D(options...))
    {
    }

    std::string generate_new_token()
    {
      std::ostringstream os;
      std::random_device rd;
      os << std::hex << rd() << rd() << rd() << rd();
      return os.str();
    }
    
    cookie_token make(request_type* req, response_type* resp)
    {
      std::string token;
      std::string key = "sl_token";
      auto* cookie = new mimosa::http::Cookie;

      auto& cookies = req->cookies();
      auto it = cookies.find(key);
      if (it == cookies.end())
      {
        token = generate_new_token();
        cookie->setKey(key);
        cookie->setValue(token);
        cookie->setSecure(options_.has(_Secure));
        cookie->setHttpOnly(options_.has(_Http_only));
        if (options_.has(_Expires)) cookie->setExpires(options_.get(_Expires, [] () { return ""; })());
        cookie->setPath(options_.get(_Path, "/"));
        resp->addCookie(cookie);
      }
      else
        token = it->second;

      return cookie_token(token);
    }

    sio<O...> options_;
  };

  template <typename... O>
  auto mimosa_session_cookie_middleware(O&&... opts)
  {
    return mimosa_session_cookie<O...>(opts...);
  }

  struct mimosa_silicon
  {
    typedef mimosa::http::RequestReader request_type;
    typedef mimosa::http::ResponseWriter response_type;

    template <typename T>
    auto deserialize(request_type& r, T& res) const
    {
      std::string body;
      char buf[1024];
      int n;
      while ((n = r.read(buf, 1024))) { body.append(buf, n); }
      try
      {
        json_decode(res, body);
      }
      catch (const std::runtime_error& e)
      {
        throw error::bad_request("Error when decoding procedure arguments: ", e.what());
      }

    }

    template <typename T>
    auto serialize(response_type& r, const T& res) const
    {
      std::string str = json_encode(res);
      r.write(str.data(), str.size());
    }

  };
    
  template <typename S>
  struct mimosa_handler : public mimosa::http::Handler
  {
    typedef mimosa::http::RequestReader RequestReader;
    typedef mimosa::http::ResponseWriter ResponseWriter;

    mimosa_handler(S service) : service_(service) {}
  
    bool handle(RequestReader& request,
                ResponseWriter& response) const
    {
      auto _this = const_cast<mimosa_handler<S>*>(this);
      try
      {
        _this->service_(request.location(), request, response);
      }
      catch(const error::error& e)
      {
        response.setStatus(e.status());
        std::string m = e.what();
        response.write(m.data(), m.size());
      }
      catch(const std::runtime_error& e)
      {
        std::cout << e.what() << std::endl;
        response.setStatus(500);
        std::string m = "Internal server error.";
        response.write(m.data(), m.size());
      }

      return true;
    }

  private:
    S service_;
  };

  template <typename A>
  void mimosa_json_serve(const A& api, int port)
  {
    auto service = make_service<mimosa_silicon>(api);
    mimosa_handler<decltype(service)> handler(service);
    mimosa::http::Server::Ptr server = new mimosa::http::Server;
    server->setHandler(&handler);

    if (!server->listenInet4(port))
    {
      std::cerr << "Error: Failed to listen on the port " << port << " " << ::strerror(errno) << std::endl;
      return;
    }

    //mimosa::cpuForeach([&server] {
        while (true)
          server->serveOne(0, true);
        //});
  }

}
