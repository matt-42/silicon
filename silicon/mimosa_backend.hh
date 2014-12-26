#pragma once

#include <sstream>
#include <mimosa/init.hh>
#include <mimosa/http/server.hh>
#include <mimosa/http/response-writer.hh>
#include <mimosa/http/request-reader.hh>
#include <mimosa/http/fs-handler.hh>
#include <silicon/symbols.hh>
#include <silicon/file.hh>

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

      body_stream.str(body_string);
      body_stream.seekg(0);
      body_stream.clear();
    }

    auto get_body_string() const { return body_stream.str(); }
    auto& get_body_stream() { return body_stream; }

    auto& get_params_string() { return params_string; }
    auto& get_tail_string() { return tail_string; }

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
      tail_string = params_string;
    }

    void set_tail_position(int pos) {
      tail_string.str = params_string.data() + pos;
      tail_string.len = params_string.size() - pos;
    }

    const std::string& location() { return rr_.location(); }
    
    std::istringstream body_stream;
    std::string body_string;
    stringview params_string;
    stringview tail_string;
    mh::RequestReader& rr_;
  };

  struct response
  {
    response(mh::RequestReader& rq, mh::ResponseWriter& rw) : req_(rq), resp_(rw) {}

    auto get_body_string() const { return body_stream_.str(); }
    auto& get_body_stream() { return body_stream_; }

    
    void write(const char* str, int len) {
      resp_.write(str, len);
    }
    
    void write(const std::string& s)
    {
      write(s.data(), s.size());
    }

    template <typename... T>
    auto write(const iod::sio<T...>& t)
    {
      write(json_encode(t));
    }

    template <typename... T>
    void write(const file& t)
    {
      mh::FsHandler fs("/", 0);
      fs.streamFile(req_, resp_, t.path());
    }
    
    template <typename T>
    void write(const T& t)
    {
      std::stringstream ss;
      ss << t;
      write(ss.str());
    }
    
    template <typename T>
    auto& operator<<(const T& t) { write(t); return *this; }
    
    void write_body() {
      std::string s = body_stream_.str();
      resp_.write(s.c_str(), s.size());
    }

    template <typename... O>
    void set_cookie(const std::string& key, const std::string& value, O&&... _options)
    {
      auto options = D(_options...);
      auto* cookie = new mimosa::http::Cookie;
      cookie->setKey(key);
      cookie->setValue(value);
      cookie->setSecure(options.has(_Secure));
      cookie->setHttpOnly(options.has(_Http_only));
      if (options.has(_Expires)) cookie->setExpires(options.get(_Expires, ""));
      cookie->setPath(options.get(_Path, "/"));
      resp_.addCookie(cookie);
    }

    void set_header(const std::string& key, const std::string& value) { resp_.addHeader(key, value); }
    
    void set_status(int s) { resp_.setStatus(s); }
    
    std::ostringstream body_stream_;
    mh::RequestReader& req_;
    mh::ResponseWriter& resp_;
  };

  template <typename F>
  struct mimosa_handler : public mh::Handler
  {
    mimosa_handler(F f) : fun_(f) {}

    bool handle(mh::RequestReader & request_,
                mh::ResponseWriter & response_) const
    {
      request request(request_);
      response response(request_, response_);
      fun_(request, response);
      return true;
    }
    F fun_;
  };
    
  struct mimosa_backend
  {
    mimosa_backend(int argc, char* argv[]) : listening_(false) { mimosa::init(argc, argv); }
    mimosa_backend() : listening_(false) { mimosa::deinit(); }
    
    template <typename F>
    void serve(int port, F f);

    void stop_after_next_request() { stopped_ = true; }
    bool listening() const { return listening_; }

    bool stopped_;
    bool listening_;
  };

  typedef mimosa_backend backend;
    
}

#include <silicon/mimosa_backend.hpp>
