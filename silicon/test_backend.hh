#pragma once

#include <sstream>

namespace iod
{

  typedef std::istringstream request;
  typedef std::ostringstream response;

  struct server;

  struct test_backend
  {
    typedef std::stringstream request_type;
    typedef std::stringstream response_type;

    test_backend(int argc, char* argv[]) {}

    void serve(server& s);
    void serve_one(server& s);

    void serve_test(std::string rq, server& s);
  };

  typedef test_backend backend;

}
