#pragma once

#include <silicon/middlewares/sql_session.hh>
#include <silicon/middlewares/mysql_connection.hh>

namespace sl
{
  template <typename O>
  using mysql_session = sql_session<mysql_connection, O>;  
  template <typename O>
  using mysql_session_factory = sql_session_factory<mysql_connection, O>;
}
