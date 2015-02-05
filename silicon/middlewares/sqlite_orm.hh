#pragma once

#include <silicon/middlewares/sql_orm.hh>
#include <silicon/middlewares/sqlite_connection.hh>

namespace sl
{
  template <typename O>
  using sqlite_orm = sql_orm<sqlite_connection, O>;
  template <typename O>
  using sqlite_orm_middleware = sql_orm_middleware<sqlite_connection, O>;

}
