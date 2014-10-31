#pragma once

namespace iod
{
  namespace mh = mimosa::http;

  struct mimosa_handler : public mh::Handler
  {
    mimosa_handler(http_server* s) : server_(s) {}

    bool handle(mh::RequestReader & request_,
                mh::ResponseWriter & response_) const
    {
      request request(request_);
      response response(response_);

      server_->run(request, response);
    }

    http_server* server_;
  };

  http_server::http_server(int argc, char* argv)
  {
    mimosa::init(argc, argv);
    handler_ = new mimosa_handler(this);
    server_ = new mh::Server;
  }

  void
  http_server::add_route(std::string path, handler_base handler)
  {
    handlers.push_back(handler);
    routes[path] = handlers.size() - 1;
  }

  void
  http_server::run(request& rq, response& resp)
  {
    // get rq metadata.
    iod(s::handler_id = int()) md;
    iod::json_decode(rq.body(), md); // Fixme

    if (md.handler_id < handlers.size())
      handlers[metadata.handler_id](rq, resp); // run the handler.
    else
      throw http_not_found();
  }

}
