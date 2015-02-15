#pragma once

#include <silicon/middlewares/sql_orm.hh>
#include <silicon/middlewares/mysql_connection.hh>

namespace sl
{
  template <typename O>
  using mysql_orm = sql_orm<mysql_connection, O>;  
  template <typename O>
  using mysql_orm_factory = sql_orm_factory<mysql_connection, O>;
}
