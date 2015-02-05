#pragma once

#include <iod/di.hh>
#include <iod/utils.hh>

namespace sl
{
  using namespace iod;

  template <typename ORM, typename... T>
  auto sql_crud(T&&... _opts)
  {
    typedef typename ORM::instance_type ORMI;
    
    typedef typename ORMI::object_type O; // O without primary keys for create procedure.
    typedef typename ORMI::PKS PKS; // Object with only the primary keys for the delete procedure.

    typedef decltype(sql_orm_internals::remove_computed_fields(std::declval<O>())) update_type;
    typedef decltype(sql_orm_internals::remove_pks(std::declval<update_type>())) insert_type;
    
    auto call_callback = [] (auto& f, O& o, auto& deps)
    {
      return apply(f, forward(o), deps.deps, di_call);
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

    return D(

      _get_by_id(_id = int()) = [=] (auto params, ORMI& orm,
                                     di::dependencies_of<decltype(read_access)>& ra_deps)
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
      
      _create = [=] (insert_type obj, ORMI& orm,
                     di::dependencies_of<decltype(validate)>& v_deps,
                     di::dependencies_of<decltype(on_create_success)>& oc_deps,
                     di::dependencies_of<decltype(write_access)>& wa_deps,
                     di::dependencies_of<decltype(before_create)>& bc_deps)
    {
      O o;
      o = obj;
      call_callback(before_create, o, bc_deps);
      if (call_callback(write_access, o, wa_deps) and
          call_callback(validate, o, v_deps))
      {
        int new_id = orm.insert(o);
        call_callback(on_create_success, o, oc_deps);
        return D(_id = new_id);
      }
      else
        throw error::unauthorized("Not enough priviledges to edit this object");
    },

      _update = [=] (update_type obj, ORMI& orm,
                     di::dependencies_of<decltype(validate)>& v_deps,
                     di::dependencies_of<decltype(on_update_success)>& ou_deps,
                     di::dependencies_of<decltype(write_access)>& wa_deps,
                     di::dependencies_of<decltype(before_update)>& bu_deps)
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

      _destroy = [=] (PKS params,
                      ORMI& orm,
                      di::dependencies_of<decltype(on_destroy_success)>& od_deps,
                      di::dependencies_of<decltype(write_access)>& wa_deps)
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
    });
  }
  
  
}
