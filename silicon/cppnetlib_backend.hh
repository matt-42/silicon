#pragma once

namespace iod
{
  namespace http = boost::network::http;


  struct request

  struct http_server;


  struct cnl_handler
  {
    cnl_handler(http_server* s) : server_(s) {}

    void operator()(http_server::request const& request_,
                    http_server::response& response_);

    http_server* server_;
  };

  typedef http::server<cnl_handler> cnl_http_server;

  struct http_server
  {
    http_server(int argc, char* argv);

    void add_route(std::string path, handler_base handler);
    void run(request& rq, response& resp);
    void serve();

    cnl_http_server server_;
  };

  struct request_body_stream : std::stringstream
  {
    request_body_stream(http::server::request& r);

    template <typename... T>
    request_body_stream& operator>>()(iod_object<T...>& o);
  };

  struct request
  {
    request(http::server::request& r);

    request_body_stream& body_stream();

    http::server::request& request_;
    request_body_stream body_stream_;
  };

  struct response_body_stream : std::stringstream
  {
    response_body_stream(http::server::response& r);

    template <typename... T>
    response_body_stream& operator>>()(iod_object<T...>& o);
  };

  struct response
  {
    response(http::server::response& r);

    response_body_stream& body_stream();

    http::server::response& response_;
    response_body_stream body_stream_;
  };

}
