#pragma once

#include <sstream>
#include <boost/network/protocol/http/server.hpp>

#include <iod/json.hh>

namespace iod
{

  struct cppnet_handler;
  namespace bnh = boost::network::http;
  typedef bnh::server<cppnet_handler> cnl_server;
  typedef cnl_server::request cnl_request;
  typedef cnl_server::response cnl_response;

  struct request
  {
    request(cnl_request const& rr) : rr_(rr) {
      body_string = rr.body;
      body_stream.str(body_string);
      body_stream.seekg(0);
      body_stream.clear();
    }
    
    auto get_body_string() const { return body_stream.str(); }
    auto& get_body_stream() { return body_stream; }

    std::istringstream body_stream;
    std::string body_string;
    cnl_request const& rr_;
  };

  struct response
  {
    response() {}

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

    std::ostringstream body_stream_;
  };

  struct cppnet_handler {

    template <typename F>
    cppnet_handler(F f) : fun_(f) {}

    void operator() (cnl_request const &request_,
                     cnl_response &response_) {

      request request(request_);
      response response;
      fun_(request, response);
      response_ = cnl_server::response::stock_reply(
         cnl_server::response::ok, response.get_body_string());
    }
      
    void log(cnl_server::string_type const &info) {
      std::cerr << "ERROR: " << info << '\n';
    }

    std::function<void(request&, response&)> fun_;
  };

  struct cppnet_backend
  {
    cppnet_backend(int argc, char* argv[]) { }
    cppnet_backend() {  }
    
    template <typename F>
    void serve(F f);
  };

  typedef cppnet_backend backend;

  
  template <typename F>
  void cppnet_backend::serve(F f)
  {
    namespace http = boost::network::http;

    cppnet_handler handler(f);
 
    namespace utils = boost::network::utils;
    cnl_server::options options(handler);
    cnl_server server_(
      options.address("0.0.0.0")
      .thread_pool(boost::make_shared<boost::network::utils::thread_pool>(4))
      .port("8888"));
    server_.run();
    std::cout << "serve end. " << std::endl;
  }
  
}
