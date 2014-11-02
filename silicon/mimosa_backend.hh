#pragma once

#include <sstream>
#include <mimosa/init.hh>
#include <mimosa/http/server.hh>
#include <mimosa/http/response-writer.hh>
#include <mimosa/http/request-reader.hh>

#include <iod/json.hh>

namespace iod
{
  namespace mh = mimosa::http;

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

    std::istringstream body_stream;
    std::string body_string;
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
      fun_(request, response);
      response.write_body();
      response.rw_.sendHeader();
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
