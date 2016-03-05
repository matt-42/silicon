#pragma once

#include <iod/di.hh>
#include <iod/utils.hh>
#include <silicon/middlewares/sql_orm.hh>
#include <silicon/error.hh>

namespace sl
{
  using namespace iod;

  template <typename O, typename F>
  using handler_deps = di::dependencies_of_<tuple_remove_elements_t<callable_arguments_tuple_t<F>,
                                                                    O, O&, const O&>>;

  template <typename F, typename O, typename... D>
  auto sql_crud_call_callback(F& f, O& o, std::tuple<D...>& deps)
  {
    return di_call(f, o, tuple_get_by_type<D>(deps)...);
  }
  
  template <typename ORMI, typename... T>
  auto sql_crud(T&&... _opts)
  {    
    typedef typename ORMI::object_type O; // O without primary keys for create procedure.
    typedef typename ORMI::PKS PKS; // Object with only the primary keys for the delete procedure.

    typedef sql_orm_internals::remove_read_only_fields_t<O> update_type;
    typedef sql_orm_internals::remove_auto_increment_t<update_type> insert_type;
    
    auto call_callback = [] (auto& f, O& o, auto& deps)
    {
      return sql_crud_call_callback(f, o, deps.deps);
      //return iod::apply(f, o, deps.deps, o, di_call(f, o, deps.deps, di_call);
    };

    auto opts = D(_opts...);
    auto read_access = opts.get(_read_access, [] () { return true; });
    auto write_access = opts.get(_write_access, [] () { return true; });
    auto validate = opts.get(_validate, [] () { return true; });
    auto on_create_success = opts.get(_on_create_success, [] () {});
    auto on_destroy_success = opts.get(_on_destroy_success, [] () {});
    auto on_update_success = opts.get(_on_update_success, [] () {});

    auto before_update = opts.get(_before_update, [] () {});
    auto before_create = opts.get(_before_create, [] () {});
    
    //auto before_destroy = opts.get(_before_destroy, [] () {});
    typedef handler_deps<O, decltype(before_create)> bc_deps_t;
    typedef handler_deps<O, decltype(write_access)> wa_deps_t;
    typedef handler_deps<O, decltype(validate)>& v_deps_t;
    typedef handler_deps<O, decltype(on_create_success)>& oc_deps_t;
    typedef handler_deps<O, decltype(on_update_success)> ou_deps_t;
    typedef handler_deps<O, decltype(before_update)> bu_deps_t;

    return http_api(
      GET / _get_by_id * get_parameters(_id = int()) = [=] (auto params, ORMI& orm,
                                                      handler_deps<O, decltype(read_access)>& ra_deps)
    {
      O o;
      if (!orm.find_by_id(params.id, o))
        throw error::not_found("Object not found");
      if (call_callback(read_access, o, ra_deps))
        return o;
      else
        throw error::unauthorized("Not enough priviledges to edit this object");
    }
      ,
      
      POST / _create * post_parameters(insert_type()) =
      [=] (auto obj, ORMI& orm,
           v_deps_t& v_deps,
           oc_deps_t& oc_deps,
           bc_deps_t& bc_deps
        )
    {
      O o;
      o = obj;
      call_callback(before_create, o, bc_deps);
      if (call_callback(validate, o, v_deps))
      {
        int new_id = orm.insert(o);
        call_callback(on_create_success, o, oc_deps);
        return D(_id = new_id);
      }
      else
        throw error::unauthorized("Not enough priviledges to edit this object");
    },

      POST / _update * post_parameters(update_type()) = [=] (auto obj, ORMI& orm,
                                                    v_deps_t& v_deps,
                                                    ou_deps_t& ou_deps,
                                                    wa_deps_t& wa_deps,
                                                    bu_deps_t& bu_deps)
    {
      O o;
      if (!orm.find_by_id(obj.id, o))
        throw error::not_found("Object with id ", obj.id, " not found");

      if (!call_callback(write_access, o, wa_deps))
        throw error::unauthorized("Not enough priviledges to edit this object");

      // Update
      o = obj;      
      if (!call_callback(validate, o, v_deps))
        throw error::bad_request("Invalid update parameters");
      
      call_callback(before_update, o, bu_deps);
      orm.update(o);
      call_callback(on_update_success, o, ou_deps);
    },

      POST / _destroy * post_parameters(PKS()) = [=] (auto params,
                      ORMI& orm,
                      handler_deps<O, decltype(on_destroy_success)>& od_deps,
                      wa_deps_t& wa_deps)
    {
      O o;
      if (!orm.find_by_id(params.id, o))
        throw error::not_found("Delete: Object with id ", params.id, " not found");
      
      if (call_callback(write_access, o, wa_deps))
      {
        orm.destroy(params);
        call_callback(on_destroy_success, o, od_deps);
      }
      else
        throw error::unauthorized("Cannot delete this object");
    }
      );
  }
  
  
}
