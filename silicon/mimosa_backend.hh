#pragma once

#include <sstream>
#include <mimosa/init.hh>
#include <mimosa/http/server.hh>
#include <mimosa/http/response-writer.hh>
#include <mimosa/http/request-reader.hh>
#include <silicon/symbols.hh>

#include <iod/json.hh>

namespace iod
{
  namespace mh = mimosa::http;

  using s::_Secure;
  using s::_Path;
  using s::_Http_only;
  using s::_Expires;
  
  struct request
  {
    request(mh::RequestReader& rr) : rr_(rr) {
      int n;
      char buf[1024];
      while ((n = rr_.read(buf, 1024))) { body_string.append(buf, n); }
      // std::cout << body_string << std::endl;
      body_stream.str(body_string);
      body_stream.seekg(0);
      body_stream.clear();
    }

    auto get_body_string() const { return body_stream.str(); }
    auto& get_body_stream() { return body_stream; }

    auto& get_params_string() { return params_string; }

    bool get_cookie(const std::string& key, std::string& value)
    {
      auto it = rr_.cookies().find(key);
      if (it != rr_.cookies().end())
      {
        value = it->second;
        return true;
      }
      else return false;
    }
    
    void set_params_position(int pos) {
      params_string.str = body_string.c_str() + pos;
      params_string.len = body_string.size() - pos;
    }
    
    std::istringstream body_stream;
    std::string body_string;
    stringview params_string;
    mh::RequestReader& rr_;
  };

  struct response
  {
    response(mh::ResponseWriter& rw) : rw_(rw) {}

    auto get_body_string() const { return body_stream_.str(); }
    auto& get_body_stream() { return body_stream_; }

    template <typename T>
    auto operator<<(const T& t)
    {
      body_stream_ << t;
    }

    template <typename... T>
    auto operator<<(const iod::sio<T...>& t)
    {
      json_encode(t, body_stream_);
    }
    
    void write_body() {
      std::string s = body_stream_.str();
      rw_.write(s.c_str(), s.size());
    }

    template <typename... O>
    void set_cookie(const std::string& key, const std::string& value, O&&... _options)
    {
      auto options = D(_options...);
      auto* cookie = new mimosa::http::Cookie;
      cookie->setKey(key);
      cookie->setKey(value);
      cookie->setSecure(options.has(_Secure));
      cookie->setHttpOnly(options.has(_Http_only));
      if (options.has(_Expires)) cookie->setExpires(options.get(_Expires, ""));
      cookie->setPath(options.get(_Path, "/"));
      rw_.addCookie(cookie);
    }
    
    void set_status(int s) { rw_.setStatus(s); }
    
    std::ostringstream body_stream_;
    mh::ResponseWriter& rw_;
  };

  template <typename F>
  struct mimosa_handler : public mh::Handler
  {
    mimosa_handler(F f) : fun_(f) {}

    bool handle(mh::RequestReader & request_,
                mh::ResponseWriter & response_) const
    {
      request request(request_);
      response response(response_);
      // std::cout << "before" << std::endl;
      // std::cout << &request_ << std::endl;
      // std::cout << &response_ << std::endl;
      fun_(request, response);
      // std::cout << "after" << std::endl;
      response.rw_.setContentLength(response.get_body_string().size());
      // std::cout << response.get_body_string().size() << std::endl;
      response.write_body();
      return true;
    }
    F fun_;
  };
    
  struct mimosa_backend
  {
    mimosa_backend(int argc, char* argv[]) { mimosa::init(argc, argv); }
    mimosa_backend() { mimosa::deinit(); }
    
    template <typename F>
    void serve(F f);
  };

  typedef mimosa_backend backend;
    
}

#include <silicon/mimosa_backend.hpp>
