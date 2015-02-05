#pragma once

#include <sstream>
#include <iod/utils.hh>
#include <silicon/symbols.hh>

namespace sl
{

  using s::_primary_key;
  using s::_primary_key_t;

  using s::_computed_t;
  using s::_computed;
  
  namespace sql_orm_internals
  {
    template <typename M>
    struct is_primary_key
    {
      typedef typename std::decay_t<M>::attributes_type attrs;
      static const bool value = has_symbol<attrs, _primary_key_t>::value;
    };
    auto remove_pks = [] (auto o)
    {
      return foreach(o) | [] (auto& m) {
        typedef typename std::decay_t<decltype(m)>::attributes_type attrs;
        return ::iod::static_if<!has_symbol<attrs, _primary_key_t>::value>(
          [&] () { return m; },
          [&] () {});
      };
    };
    auto get_pks = [] (auto o)
    {
      return foreach(o) | [] (const auto& m) {
        return static_if<!is_primary_key<decltype(m)>::value>(
          [&] () {},
          [&] () { return m; });
      };
    };

    auto remove_computed_fields = [] (const auto& o)
    {
      return foreach(o) | [&] (auto& m)
      {
        typedef typename std::decay_t<decltype(m)>::attributes_type attrs;
        return ::iod::static_if<!has_symbol<attrs, _computed_t>::value>(
          [&] () { return m; },
          [&] () {});
      };
    };
    
  }
  
  template <typename C, typename O>
  struct sql_orm
  {
    typedef O object_type;

    // O without primary keys for create procedure.
    typedef decltype(sql_orm_internals::remove_pks(std::declval<O>())) O_WO_PKS;

    // Object with only the primary keys for the delete procedure.
    typedef decltype(sql_orm_internals::get_pks(std::declval<O>())) PKS;

    sql_orm(const std::string& table, C& con) : table_name_(table), con_(con) {}

    int find_by_id(int id, O& o)
    {
      return con_("SELECT * from " + table_name_ + " where id == ?", id) >> o;
    }

    template <typename... T>
    auto select_where(T&&... t)
    {
      return con_("SELECT * from " + table_name_ + " where ", t...);
    }

    template <typename N>
    int insert(const N& o)
    {
      // save all fields except primary keys.
      // The db will automatically fill auto increment keys.

      std::stringstream ss;
      std::stringstream vs;
      ss << "INSERT into " << table_name_ << "(";

      bool first = true;      
      auto values = foreach(O_WO_PKS()) | [&] (auto& m) {
        return static_if<!sql_orm_internals::is_primary_key<decltype(m)>::value>(
          [&] () {
            if (!first) { ss << ","; vs << ","; }
            first = false;
            ss << m.symbol().name();
            vs << "?";
            return m.symbol() = m.symbol().member_access(o);
          },
          [] () {
          });
      };

      ss << ") VALUES (" << vs.str() << ")";
      if (apply(ss.str(), values, con_).exec())
        return con_.last_insert_rowid();
      else return false;
    };

    int update(O& o)
    {
      std::stringstream ss;
      ss << "UPDATE " << table_name_ << " SET ";

      bool first = true;      
      auto values = foreach(o) | [&] (auto& m) {
        return static_if<!sql_orm_internals::is_primary_key<decltype(m)>::value>(
          [&] () {
            if (!first) ss << ",";
            first = false;
            ss << m.symbol().name() << " = ?";
            return m;
          },
          [] () {});
      };

      ss << " WHERE ";
      
      first = true;
      const PKS& pks = *(PKS*)0;
      auto pks_values = foreach(pks) | [&] (auto& m) {
        if (!first) ss << " and ";
        first = false;
        ss << m.symbol().name() << " == ? ";
        return m.symbol() = o[m.symbol()];
      };
      
      return apply(ss.str(), values, pks_values, con_).exec();
    }

    template <typename T>
    int destroy(const T& o)
    {
      std::stringstream ss;
      ss << "DELETE from " << table_name_ << " WHERE ";

      bool first = true;
      auto values = foreach(PKS()) | [&] (auto& m) {
        if (!first) ss << " and ";
        first = false;
        ss << m.symbol().name() << " == ? ";
        return m.symbol() = o[m.symbol()];
      };

      return apply(ss.str(), values, con_).exec();
    }
    
    std::string table_name_;
    sqlite_connection con_;
  };

  
  template <typename C, typename O>
  struct sql_orm_middleware
  {
    typedef O object_type;
    typedef sql_orm<C, O> instance_type;

    sql_orm_middleware(const std::string& table_name) : table_name_(table_name) {}

    void initialize(C& c)
    {
      std::stringstream ss;
      ss << "CREATE TABLE if not exists " <<  table_name_ << " (";

      bool first = true;
      foreach(O()) | [&] (auto& m)
      {
        if (!first) ss << ", ";
        ss << m.symbol().name() << " " << c.type_to_string(m.value());

        if (m.attributes().has(_primary_key))
          ss << " PRIMARY KEY NOT NULL";

        first = false;
      };
      ss << ");";
      c(ss.str()).exec();
    }
    
    auto instantiate(C& con) {
      return sql_orm<C, O>(table_name_, con);
    };

    std::string table_name_;
  };

}
