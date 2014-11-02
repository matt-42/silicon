#pragma once

namespace iod
{
  
  template <typename F>
  void mimosa_backend::serve(F f)
  {
    mimosa_handler<F> handler(f);
    mh::Server::Ptr server = new mh::Server;
    server->setHandler(&handler);

    if (!server->listenInet4(8888))
    {
      std::cerr << "failed to listen on the port " << 8888 << " " << ::strerror(errno) << std::endl;
      exit(1);
    }

    while(true)
      server->serveOne(0, true);
    std::cout << "serve end. " << std::endl;
  }
    
}
