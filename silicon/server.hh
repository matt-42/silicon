#pragma once

#include <string>
#include <vector>

#include <iod/sio.hh>
#include <iod/json.hh>

iod_define_symbol(handler_id, _Handler_id);

namespace iod
{

  template <typename C>
  struct handler;

  void
  run_handler(const handler<std::string>& handler,
              request& request, response& response);

  struct http_error
  {
    virtual std::string what() const { return "Http error."; }
  };

  struct http_not_found : public http_error
  {
    virtual std::string what() const { return "Http error not found."; }
  };

  struct handler_base
  {
    virtual void operator()(request& request, response& response) = 0;
  };

  template <typename C>
  struct handler : public handler_base
  {
    handler(std::string n, C c) : name_(n), content_(c) {}

    virtual void operator()(request& request, response& response)
    {
      try
      {
        run_handler(*this, request, response);
      }
      catch(const http_error& e)
      {
        response << e.what();
      }
    }

    std::string name_;
    C content_;
  };
  
  struct handler_creator
  {
    handler_creator(server* s, std::string name) : s_(s), name_(name) {}

    template <typename C>
    void operator=(C content);

    server* s_;
    std::string name_;
  };

  struct server
  {
    server(int argc, char* argv[])
      : backend_(argc, argv)
    {}

    auto operator[](std::string route_path)
    {
      return handler_creator(this, route_path);
    }

    template <typename F>
    auto add_procedure(std::string name, F f)
    {
      handlers.push_back(new handler<F>(name, f));
      return *this;
    }

    void handle(request& request,
                response& response)
    {
      std::string body = std::string(std::istreambuf_iterator<char>(request),
                                    std::istreambuf_iterator<char>());

      // get rq metadata.
      auto md = iod::D(s::_Handler_id = int());
      int position;
      iod::json_decode(md, body, position); // Fixme implement json decode on stream.

      request.seekg(position);
      body = std::string(body.begin() + position, body.end());
      if (md.handler_id < handlers.size() and md.handler_id >= 0)
        (*handlers[md.handler_id])(request, response); // run the handler.
      else
        response << "Http error: 404 not found.";
      //throw new http_not_found();
    }

    void serve()
    {
      backend_.serve(*this);
    }

    void serve_one()
    {
      backend_.serve_one(*this);
    }

  private:
    backend backend_;
    std::vector<handler_base*> handlers;
  };

  
  template <typename C>
  void handler_creator::operator=(C content)
  {
    s_->add_procedure(name_, content);
  }

}

#include <silicon/run_handler.hh>
