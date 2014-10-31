#pragma once

namespace iod
{

  test_backend::test_backend()
  {
  }

  void test_backend::serve_one(server& s) {}
  void test_backend::serve(server& s) {}

  std::string test_backend::serve_test(std::string rq, server& handler)
  {
    std::stringstream response;
    std::stringstream request(rq);
      
    handler(request, response);
    return response.str();
  }

}
