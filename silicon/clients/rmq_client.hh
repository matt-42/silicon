#pragma once

#include <memory>

#include <iod/sio_utils.hh>

#include <silicon/utils.hh>
#include <silicon/rmq_route.hh>
#include <silicon/backends/rabbitmq.hh>

namespace sl
{
namespace rmq
{
  template <typename C, typename S, typename A>
  auto publish_json(C client, S route, A const & args)
  {
    std::string json;

    iod::static_if<(route.params._size > 0 and A::_size > 0)>(
      [&] (auto args)
      {
        auto params = iod::intersect(args, route.params);
        json = json_encode(params);
      },
      [](auto) {}, args);

    amqp_bytes_t body = amqp_cstring_bytes(json.c_str());

    amqp_basic_properties_t props;
    memset(&props, 0, sizeof props);

    props._flags = AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.delivery_mode = 2; // persistent

    props._flags |= AMQP_BASIC_CONTENT_TYPE_FLAG;
    props.content_type = amqp_cstring_bytes("application/json");

    die_on_error(amqp_basic_publish(client->conn, 1,
                                    amqp_cstring_bytes(route.exchange_as_string()),
                                    amqp_cstring_bytes(route.routing_key_as_string().c_str()),
                                    0,
                                    0,
                                    &props, body),
                 "amqp.basic.publish");

  }


  template <typename C, typename R, typename Ret, typename... O>
  struct generic_client_call
  {
    generic_client_call(C c, R r, O &&... opts):
      client(c),
      route(r)
    {
      auto options = D(opts...);

      if (route.has_qn)
      {
        auto options = D(opts...);
        amqp_channel_t chan = 1;

        amqp_queue_declare(client->conn, chan,
                           amqp_cstring_bytes(route.queue_name_as_string().c_str()),
                           options.get(s::_passive, false),
                           options.get(s::_durable, false),
                           options.get(s::_exclusive, false),
                           options.get(s::_auto_delete, false),
                           amqp_empty_table);

        die_on_amqp_error(amqp_get_rpc_reply(client->conn), "amqp.queue.declare");
      }
    }

    template <typename... A>
    auto operator()(A... args)
    {
      publish_json(client, route, D(args...));
    }

    C client;
    R route;
  };

  template <typename Ret, typename C, typename R, typename... O>
  auto create_client_call(C & c, R route, O &&... opts)
  {
    return generic_client_call<C, R, Ret, O...>(c, route, opts...);
  }

  template <typename S>
  decltype(auto) process_roots(S o)
  {
    auto res = sio_iterate(o, D(s::_root = int(), s::_methods = iod::sio<>())) | [] (auto m, auto prev)
    {
      // if m.value() is a sio, recurse.
      return iod::static_if<iod::is_sio<decltype(m.value())>::value>(
        [&] (auto m)
        {
        return D(s::_root = prev.root,
                 s::_methods = cat(prev.methods,
                                   D(m.symbol() = process_roots(m.value()))));
        },

        // if not, process a leave.
        [&] (auto m)
        {
      
          // if m is root
          return iod::static_if<std::is_same<decltype(m.symbol()), s::_silicon_root____t>::value>(
            [&] (auto m, auto prev)
            {
              return D(s::_root = m.value(), s::_methods = prev);
            },
            [&] (auto m, auto prev)
            {
              // else
              return D(s::_root = prev.root, s::_methods = cat(D(m), prev.methods));
            }, m, prev);
        }, m);
    };

    return iod::static_if<std::is_same<std::decay_t<decltype(res.root)>, int>::value>(
      [] (auto res)
      {
        return res.methods;
      },
      [] (auto res)
      {
        return make_client_methods_with_root(res.root, res.methods);
      }, res);
  }
  
  template <typename C, typename A, typename PR, typename... O>
  auto generate_client_methods(C & c, A api, PR parent_route, O &&... opts)
  {
    static_assert(iod::is_tuple<decltype(api)>::value, "api should be a tuple.");

    auto tu = foreach(api) | [&] (auto m)
    {
      return iod::static_if<iod::is_tuple<decltype(m.content)>::value>(
        [&] (auto m)
        { // If tuple, recursion.
          return generate_client_methods(c, m.content, parent_route.rk_append(m.route));
        },
        [&] (auto m)
        { // Else, register the procedure.
          typedef std::remove_reference_t<decltype(m.content)> V;
          typedef typename V::function_type F;
          typename V::arguments_type arguments;

          auto route = m.route;
          auto cc = create_client_call<iod::callable_return_type_t<F>>(c, route, opts...);
          auto st = filter_symbols_from_tuple(route.routing_key);
          auto st2 = std::tuple_cat(std::make_tuple(exchange_to_symbol(route.exchange)), st);

          return iod::static_if<(std::tuple_size<decltype(m.route.routing_key)>() != 0)>(
            [&] ()
            {
              return symbol_tuple_to_sio(&st2, cc);
            },
            [&] ()
            {
              auto p = std::tuple_cat(st2, std::make_tuple(s::_silicon_root___));
              return symbol_tuple_to_sio(&p, cc);
            });
        }, m);
    };

    // If in the tuple tu, there is a method without a path, this is the root
    // and we must make it accessible via the operator() of the result object.
    auto tu2 = deep_merge_sios_in_tuple(tu);
    return process_roots(tu2);
  }

  template <typename A, typename... O>
  auto json_producer(A const & api, std::string const & host, unsigned short port, O &&... opts)
  {
    auto options = D(opts...);
    auto c = std::make_shared<utils::tcp_socket>(host, port, opts...);

    // login
    die_on_amqp_error(amqp_login(c->conn, "/", 0, 131072, 0,
                                 AMQP_SASL_METHOD_PLAIN,
                                 options.username.c_str(), options.password.c_str()),
                      "amqp.login");

    // open channel
    amqp_channel_open(c->conn, 1);
    die_on_amqp_error(amqp_get_rpc_reply(c->conn),
                      "amqp.channel.open");

    return generate_client_methods(c, api, route<>(), opts...);
  }
}
}
