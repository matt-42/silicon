#pragma once

#include <mimosa/cpu-foreach.hh>

namespace iod
{
  
  template <typename F>
  void mimosa_backend::serve(int port, F f)
  {
    stopped_ = false;
    mimosa_handler<F> handler(f);
    mh::Server::Ptr server = new mh::Server;
    server->setHandler(&handler);

    if (!server->listenInet4(port))
    {
      std::cerr << "Error: Failed to listen on the port " << port << " " << ::strerror(errno) << std::endl;
      return;
    }
    listening_ = true;
    // mimosa::cpuForeach([server, this] {
        while (!this->stopped_)
          server->serveOne();
      // });
    
  }
    
}
