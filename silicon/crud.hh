#pragma once

#include <silicon/run_handler.hh>

namespace iod
{

  template <typename ORM, typename S, typename... T>
  void setup_crud(S& server, ORM& orm, T&&... _opts)
  {
    typedef typename ORM::instance_type ORMI;
    
    typedef typename ORMI::object_type O; // O without primary keys for create procedure.
    typedef typename ORMI::O_WO_PKS O2; // O without primary keys for create procedure.
    typedef typename ORMI::PKS PKS; // Object with only the primary keys for the delete procedure.

    auto call_callback = [] (auto& f, O& o, auto& deps)
    {
      return apply(f, forward(o), deps.deps, call_with_di);
    };

    auto opts = D(_opts...);
    auto read_access = opts.get(_Read_access, [] () { return true; });
    auto write_access = opts.get(_Write_access, [] () { return true; });
    auto validate = opts.get(_Validate, [] () { return true; });
    auto on_create_success = opts.get(_On_create_success, [] () {});
    auto on_destroy_success = opts.get(_On_destroy_success, [] () {});
    auto on_update_success = opts.get(_On_update_success, [] () {});
    
    std::string prefix = opts.prefix;
    
    server[prefix + "_get_by_id"](_Id = int()) = [&] (auto params, ORMI& orm,
                                                      dependencies_of<decltype(read_access)>& ra_deps)
    {
      O o;
      if (!orm.find_by_id(params.id, o))
        throw error::not_found("Object not found");
      if (call_callback(read_access, o, ra_deps))
        return o;
      else
        throw error::unauthorized("Not enough priviledges to edit this object");
    };
    
    server[prefix + "_create"] = [&] (O2 obj, ORMI& orm,
                                      dependencies_of<decltype(validate)>& v_deps,
                                      dependencies_of<decltype(on_create_success)>& oc_deps,
                                      dependencies_of<decltype(write_access)>& wa_deps)
    {
      O o;
      o = obj;
      if (call_callback(write_access, o, wa_deps) and
          call_callback(validate, o, v_deps))
      {
        int new_id = orm.insert(o);
        call_callback(on_create_success, o, oc_deps);
        return D(_Id = new_id);
      }
      else
        throw error::unauthorized("Not enough priviledges to edit this object");
    };

    server[prefix + "_update"] = [&] (O obj, ORMI& orm,
                                      dependencies_of<decltype(validate)>& v_deps,
                                      dependencies_of<decltype(on_update_success)>& ou_deps,
                                      dependencies_of<decltype(write_access)>& wa_deps)
    {
      if (call_callback(write_access, obj, wa_deps) and
          call_callback(validate, obj, v_deps))
      {
        orm.update(obj);
        call_callback(on_update_success, obj, ou_deps);
      }
      else
        throw error::unauthorized("Not enough priviledges to edit this object");
    };

    server[prefix + "_delete"] = [&] (PKS params,
                                      ORMI& orm,
                                      dependencies_of<decltype(validate)>& v_deps,
                                      dependencies_of<decltype(on_destroy_success)>& od_deps,
                                      dependencies_of<decltype(write_access)>& wa_deps)
    {
      O o;
      o = params;
      if (call_callback(write_access, o, wa_deps) and
          call_callback(validate, o, v_deps))
      {
        orm.destroy(params);
        call_callback(on_destroy_success, o, od_deps);
      }
      else
        throw error::unauthorized("Cannot delete this object");
    };
  }
  
}
