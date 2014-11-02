#pragma once

#include <sstream>
#include <iod/json.hh>

namespace iod
{

  struct request
  {
    request(std::string s) : body_stream(s) {}
    auto get_body_string() const { return body_stream.str(); }
    auto& get_body_stream() { return body_stream; }
    std::istringstream body_stream;
  };

  struct response
  {
    response() {}

    void set_body_string(const std::string& s) { body_stream.str(s); }

    template <typename T>
    auto operator<<(const T& t)
    {
      body_stream << t;
    }

    template <typename... T>
    auto operator<<(const iod::sio<T...>& t)
    {
      json_encode(t, body_stream);
    }
    
    auto get_body_string() const { return body_stream.str(); }
    
    std::ostringstream body_stream;
  };

  struct server;

  struct test_backend
  {
    test_backend(int argc, char* argv[]) {}

    void serve(server& s);
    void serve_one(server& s);

    void serve_test(std::string rq, server& s);
  };

  typedef test_backend backend;

}
