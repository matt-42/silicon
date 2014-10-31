#pragma once

namespace iod
{

  void
  cnl_handler::operator()(http_server::request const& request_,
                          http_server::response& response_)
  {
    request request(request_);
    response response(response_);
    server_->run(request, response);
  }

  // http_server implementation.
  // ==================================

  http_server::http_server(int argc, char* argv)
    : handler_({this}),
      server_(handler_)
  {
  }

  void
  http_server::add_route(std::string path, handler_base handler)
  {
    handlers.push_back(handler);
    routes[path] = handlers.size() - 1;
  }

  void
  http_server::serve()
  {
    server_.run();
  }

  void
  http_server::run(request& rq, response& resp)
  {
    // get rq metadata.
    iod(s::handler_id = int()) md;
    rq.body_stream() >> md;

    if (md.handler_id < handlers.size())
      handlers[metadata.handler_id](rq, resp); // run the handler.
    else
      throw http_not_found();
  }

  // request_body_stream implementation.
  // ======================================

  request_body_stream::request_body_stream(http::server::request& r)
    : std::stringstream(r.body())
  {
  }

  template <typename... T>
  request_body_stream&
  request_body_stream::operator>>()(iod_object<T...>& o)
  {
    iod::json_decode(o, *this);
  }

  // request implementation.
  // ======================================

  request::request(http::server::request& r)
    : request_(r),
      body_stream_(r)
  {}

  request_body_stream&
  request::body_stream()
  {
    return body_stream_;
  }


  // response_body_stream implementation.
  // ======================================

  response_body_stream::response_body_stream(http::server::response& r)
    : std::stringstream(r.body())
  {
  }

  template <typename... T>
  response_body_stream&
  response_body_stream::operator<<()(iod_object<T...>& o)
  {
    iod::json_encode(o, *this);
  }

  // response implementation.
  // ======================================

  response::response(http::server::response& r)
    : response_(r),
      body_stream_(r)
  {}

  response_body_stream&
  response::body_stream()
  {
    return body_stream_;
  }

}
