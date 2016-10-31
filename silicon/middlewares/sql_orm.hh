#pragma once

#include <iostream>
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
      typedef std::decay_t<decltype(a)> A; 
      return foreach2(o) | [&] (auto& m)
      {
        typedef typename std::decay_t<decltype(m)>::attributes_type attrs;
        return ::iod::static_if<!has_symbol<attrs, A>::value>(
          [&] () { return m; },
          [&] () {});
      };
    };

    auto extract_members_with_attribute = [] (const auto& o, const auto& a)
    {
      typedef std::decay_t<decltype(a)> A; 
      return foreach2(o) | [&] (auto& m)
      {
        typedef typename std::decay_t<decltype(m)>::attributes_type attrs;
        return ::iod::static_if<has_symbol<attrs, A>::value>(
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
    template <typename T>
    using remove_primary_keys_t = decltype(remove_members_with_attribute(std::declval<T>(), _primary_key));

  }
  
  template <typename C, typename O>
  struct sql_orm
  {
    typedef O object_type;

    // O without auto increment for create procedure.
    typedef sql_orm_internals::remove_auto_increment_t<O> without_auto_inc_type;

    // Object with only the primary keys for the delete and update procedures.
    typedef sql_orm_internals::extract_primary_keys_t<O> PKS;

    static_assert(!std::is_same<PKS, void>::value, "You must set at least one member of the CRUD object as primary key.");
    
    sql_orm(const std::string& table, C& con) : table_name_(table), con_(con) {}

    int find_by_id(int id, O& o)
    {
      return con_("SELECT * from " + table_name_ + " where id = ?")(id) >> o;
    }

    // save all fields except auto increment.
    // The db will automatically fill auto increment keys.
    template <typename N>
    int insert(const N& o)
    {
      std::stringstream ss;
      std::stringstream vs;
      ss << "INSERT into " << table_name_ << "(";

      bool first = true;      
      auto values = foreach(without_auto_inc_type()) | [&] (auto& m) {
        if (!first) { ss << ","; vs << ","; }
        first = false;
        ss << m.symbol().name();
        vs << "?";
        return m.symbol() = m.symbol().member_access(o);
      };

      ss << ") VALUES (" << vs.str() << ")";
      auto req = con_(ss.str());
      apply(values, req);
      return req.last_insert_id();

    };

    // Iterate on all the rows of the table.
    template <typename F>
    void forall(F f)
    {
      std::stringstream ss;
      ss << "SELECT * from " << table_name_;
      con_(ss.str())() | f;
    }

    // Update N's members except auto increment members.
    // N must have at least one primary key.
    template <typename N>
    int update(const N& o)
    {
      // check if N has at least one member of PKS.
      auto pk = intersect(o, PKS());
      static_assert(decltype(pk)::size() > 0, "You must provide at least one primary key to update an object.");
      std::stringstream ss;
      ss << "UPDATE " << table_name_ << " SET ";

      bool first = true;      
      auto values = foreach(o) | [&] (auto& m) {
        if (!first) ss << ",";
        first = false;
        ss << m.symbol().name() << " = ?";
        return m;
      };

      ss << " WHERE ";
      
      first = true;
      auto pks_values = foreach(pk) | [&] (auto& m) {
        if (!first) ss << " and ";
        first = false;
        ss << m.symbol().name() << " = ? ";
        return m.symbol() = m.value();
      };

      auto stmt = con_(ss.str());
      apply(values, pks_values, stmt);
      return stmt.last_insert_id();
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
        ss << m.symbol().name() << " = ? ";
        return m.symbol() = o[m.symbol()];
      };

      apply(values, con_(ss.str()));
    }
    
    std::string table_name_;
    C con_;
  };


  struct sqlite_connection;
  struct mysql_connection;

  template <typename C, typename O>
  struct sql_orm_factory
  {
    typedef O object_type;
    typedef sql_orm<C, O> instance_type;

    sql_orm_factory(const std::string& table_name) : table_name_(table_name) {}

    void initialize(C& c)
    {
      std::stringstream ss;
      ss << "CREATE TABLE if not exists " <<  table_name_ << " (";

      bool first = true;
      foreach(O()) | [&] (auto& m)
      {
        if (!first) ss << ", ";
        ss << m.symbol().name() << " " << c.type_to_string(m.value());

        if (std::is_same<C, sqlite_connection>::value)
        {
          if (m.attributes().has(_auto_increment) || m.attributes().has(_primary_key))
            ss << " PRIMARY KEY ";
        }
        
        if (std::is_same<C, mysql_connection>::value)
        {
          if (m.attributes().has(_auto_increment))
            ss << " AUTO_INCREMENT NOT NULL";
          if (m.attributes().has(_primary_key))
            ss << " PRIMARY KEY ";
        }

        // To activate when pgsql_connection is implemented.
        // if (std::is_same<C, pgsql_connection>::value and
        //     m.attributes().has(_auto_increment))
        //   ss << " SERIAL ";
        
        first = false;
      };
      ss << ");";
      try
      {
        c(ss.str())();
      }
      catch (std::exception e)
      {
        std::cout << "Warning: Silicon could not create the " << table_name_ << " sql table." << std::endl
                  << "You can ignore this message if the table already exists."
                  << "The sql error is: " << e.what() << std::endl;
      }
    }
    
    sql_orm<C, O> instantiate(C& con) {
      return sql_orm<C, O>(table_name_, con);
    };

    std::string table_name_;
  };

}
