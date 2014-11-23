#include <iostream>
#include <fstream>

#include <iod/sio_utils.hh>

#include <silicon/mimosa_backend.hh>
#include <silicon/server.hh>

#include "symbols.hh"

using namespace s;

int main()
{
  using namespace iod;

  auto server = silicon();

  server["test_upload"] = [] (request& req)
  {
    std::ofstream of("test.txt");
    stringview content = req.get_tail_string();
    of.write(content.data(), content.size());
    of.close();
  };

  server.serve(9999);
}
