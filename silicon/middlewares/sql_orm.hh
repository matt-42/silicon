#pragma once

#include <sstream>
#include <iod/utils.hh>
#include <silicon/symbols.hh>

namespace sl
{

  using s::_primary_key;
  using s::_auto_increment;
  using s::_read_only;
  
  namespace sql_orm_internals
  {
    auto remove_members_with_attribute = [] (const auto& o, const auto& a)
    {
      std::decay_t<decltype(a)>
      return foreach(o) | [&] (auto& m)
      {
        typedef typename std::decay_t<decltype(m)>::attributes_type attrs;
        return ::iod::static_if<!has_symbol<attrs, std::decay_t<decltype(a)>>::value>(
          [&] () { return m; },
          [&] () {});
      };
    };

    auto extract_members_with_attribute = [] (const auto& o, const auto& a)
    {
      std::decay_t<decltype(a)>
      return foreach(o) | [&] (auto& m)
      {
        typedef typename std::decay_t<decltype(m)>::attributes_type attrs;
        return ::iod::static_if<has_symbol<attrs, std::decay_t<decltype(a)>>::value>(
          [&] () { return m; },
          [&] () {});
      };
    };

    auto extract_first_members_with_attribute = [] (const auto& o, const auto& a)
    {
      std::decay_t<decltype(a)>
      return foreach(o) | [&] (auto& m)
      {
        typedef typename std::decay_t<decltype(m)>::attributes_type attrs;
        return ::iod::static_if<has_symbol<attrs, std::decay_t<decltype(a)>>::value>(
          [&] () { return m; },
          [&] () {});
      };
    };
    
    template <typename T>
    using remove_auto_increment_t = decltype(remove_members_with_attribute(std::declval<T>(), _auto_increment));
    template <typename T>
    using remove_read_only_fields_t = decltype(remove_members_with_attribute(std::declval<T>(), _read_only));
    template <typename T>
    using extract_primary_keys_t = decltype(extract_members_with_attribute(std::declval<T>(), _primary_key));

  }
  
  template <typename C, typename O>
  struct sql_orm
  {
    typedef O object_type;

    // O without auto increment for create procedure.
    typedef sql_orm_internals::remove_auto_increment_t<O> without_auto_inc_type;

    // Object with only the primary keys for the delete and update procedures.
    typedef sql_orm_internals::get_primary_keys_t<O> PKS;

    sql_orm(const std::string& table, C& con) : table_name_(table), con_(con) {}

    int find_by_id(int id, O& o)
    {
      return con_("SELECT * from " + table_name_ + " where id == ?")(id) >> o;
    }

    template <typename N>
    int insert(const N& o)
    {
      // save all fields except auto increment.
      // The db will automatically fill auto increment keys.
      std::stringstream ss;
      std::stringstream vs;
      ss << "INSERT into " << table_name_ << "(";

      bool first = true;      
      auto values = foreach(without_auto_inc_type()) | [&] (auto& m) {
        return static_if<!sql_orm_internals::is_auto_increment<decltype(m)>::value>(
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
      auto req = con_(ss.str());
      apply(values, req);
      return req.last_insert_id();
    };

    void update(O& o)
    {
      std::stringstream ss;
      ss << "UPDATE " << table_name_ << " SET ";

      bool first = true;      
      auto values = foreach(o) | [&] (auto& m) {
        return static_if<!sql_orm_internals::is_auto_increment<decltype(m)>::value>(
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
      
      apply(values, pks_values, con_(ss.str()));
    }

    template <typename T>
    void destroy(const T& o)
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

      apply(values, con_(ss.str()));
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
          ss << " PRIMARY KEY NOT NULL ";
        if (m.attributes().has(_auto_increment))
          ss << " AUTO INCREMENT ";

        first = false;
      };
      ss << ");";
      c(ss.str())();
    }
    
    auto instantiate(C& con) {
      return sql_orm<C, O>(table_name_, con);
    };

    std::string table_name_;
  };

}
