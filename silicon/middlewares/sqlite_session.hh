#pragma once

#include <silicon/middlewares/sql_session.hh>
#include <silicon/middlewares/sqlite_connection.hh>

namespace sl
{
  template <typename O>
  using sqlite_session = sql_session<sqlite_connection, O>;  
  template <typename O>
  using sqlite_session_middleware = sql_session_middleware<sqlite_connection, O>;
}
