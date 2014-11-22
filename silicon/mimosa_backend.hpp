#pragma once

namespace iod
{
  
  template <typename F>
  void mimosa_backend::serve(int port, F f)
  {
    mimosa_handler<F> handler(f);
    mh::Server::Ptr server = new mh::Server;
    server->setHandler(&handler);

    if (!server->listenInet4(port))
    {
      std::cerr << "Error: Failed to listen on the port " << port << " " << ::strerror(errno) << std::endl;
      return;
    }

    while(true)
      server->serveOne(0, true);
  }
    
}
