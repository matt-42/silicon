#pragma once

#include <silicon/utils.hh>

namespace sl
{

  // Transform a http api into a sio.
  // Used by the clients.
  template <typename A>
  auto http_api_to_sio(A api)
  {
    auto tu = foreach(api) | [&] (auto m) {

      return static_if<is_tuple<decltype(m.content)>::value>(

        [&] (auto m) { // If tuple, recursion.
          return http_api_to_sio(m.content);
        },

        [&] (auto m) { // Else, register the procedure.
          typedef std::remove_reference_t<decltype(m.content)> V;
          typedef typename V::function_type F;
          typename V::arguments_type arguments;

          auto route = m.route;
          auto cc = m;
          auto st = filter_symbols_from_tuple(route.path);
          auto st2 = std::tuple_cat(std::make_tuple(http_verb_to_symbol(route.verb)), st);

          return static_if<(std::tuple_size<decltype(m.route.path)>() != 0)>(
            [&] () {
              return symbol_tuple_to_sio(&st2, cc);
            },
            [&] () {
              auto p = std::tuple_cat(st2, std::make_tuple(_silicon_root___));
              return symbol_tuple_to_sio(&p, cc);
            });
        }, m);

    };

    // if in the tuple tu, there is a method without a path, this is the root
    // and with must make it accessible via the operator() of the result object.
    return deep_merge_sios_in_tuple(tu);
  }

  // Transform a http api into a sio.
  // Used by the clients.
  template <typename A>
  auto ws_api_to_sio(A api)
  {
    auto tu = foreach(api) | [&] (auto m) {

      return static_if<is_tuple<decltype(m.content)>::value>(

        [&] (auto m) { // If tuple, recursion.
          return ws_api_to_sio(m.content);
        },

        [&] (auto m) { // Else, register the procedure.
          auto cc = m;
          auto st = m.route.path;
          return symbol_tuple_to_sio(&st, cc);
        }, m);

    };
    return deep_merge_sios_in_tuple(tu);
  }
    
}
