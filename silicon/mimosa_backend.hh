#pragma once

namespace iod
{
  struct http_server
  {
    http_server(int argc, char* argv);

    void add_route(std::string path, handler_base handler);
    void run(request& rq, response& resp);

  };

  struct request
  {
    operator>>(
  };
}
