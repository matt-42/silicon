#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace sl
{
  
  typedef websocketpp::server<websocketpp::config::asio> wspp_server;

  struct wspp_connection
  {
    websocketpp::connection_hdl hdl;
    wspp_server* server;

    bool operator<(const wspp_connection& a) const { return hdl.owner_before(a.hdl); }
  };

}
