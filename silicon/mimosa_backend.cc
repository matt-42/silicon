#pragma once

namespace iod
{
  namespace mh = mimosa::http;

  server::server(int argc, char* argv)
  {
    mimosa::init(argc, argv);
  }

  server::~server()
  {
    mimosa::deinit();
  }

}
