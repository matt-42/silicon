#pragma once

namespace iod
{

  template <typename O, typename S, typename... T>
  void setup_crud(S& server, T&&... opts)
  {
    typedef O2; // O without primary keys for create procedure.
    typedef PK; // Object with only the primary keys for the delete procedure.

    auto call_callback = [] (auto& f, O& o, auto& deps)
    {
      return apply(f, forward(o), deps.deps, call_with_di);
    };

    server[prefix + "/create"] = [] (O2& obj, ORM& orm,
                                     dependencies_of<decltype(write_access)>& wa_deps,
                                     dependencies_of<decltype(on_create)>& oc_deps)
    {
      O o;
      paste(obj, o);
      if (call_callback(write_access, o, wa_deps))
      {
        orm.insert(o);
        call_callback(on_create, o, oc_deps);
      }
      else
        throw error:unauthorized("Not enough priviledges to edit this object");
    };

    server[prefix + "/update"] = [] (O& obj, ORM& orm
                                     di_runner& di_run,
                                     decltype(write_access)) {
      if (!find_by_primary_key(obj, orm))
        throw error:not_found("Object not found");
      if (run(write_access, obj))
        orm.update<O>(obj);
      else
        throw error:unauthorized("Not enough priviledges to edit this object");
    };

    server[prefix + "/delete"] = [] (PK& params, 
                                     di_runner& di_run,
                                     decltype(write_access)) {
      if (run(write_access, obj))
        orm.destroy_by_primary_key<O>(params);
    };
  }
  
}
