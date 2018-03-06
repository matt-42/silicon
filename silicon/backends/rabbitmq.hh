#pragma once

# include <stdexcept>
# include <iostream>
# include <chrono>
# include <thread>

# include <amqp_tcp_socket.h>
# include <amqp.h>
# include <amqp_framing.h>

# include <iod/json.hh>

# include <silicon/symbols.hh>
# include <silicon/service.hh>

using namespace std::chrono_literals;

namespace sl
{
namespace rmq
{
  template <typename T>
  bool retry(std::string const & n, std::function<bool()> f, size_t m, T t)
  {
    for (size_t i {0}; i < m; ++i)
    {
      if(f())
        return true;

      std::cerr << n << " failed: retry in " << std::chrono::milliseconds(t).count() << "ms\n";

      std::this_thread::sleep_for(t);
    }
    return false;
  }
  auto get_string = [] (auto const & b) { return std::string(static_cast<char const *>(b.bytes), b.len); };

  void
  die_on_error(int x, std::string const & context)
  {
    if (x < 0)
      throw std::runtime_error(context + ": " + amqp_error_string2(x));
  }

  void
  die_on_amqp_error(amqp_rpc_reply_t const & x, std::string const & context)
  {
    switch (x.reply_type)
    {
      case AMQP_RESPONSE_NORMAL:
        return;

      case AMQP_RESPONSE_NONE:
        throw std::runtime_error(context + ": missing RPC reply type!");

      case AMQP_RESPONSE_LIBRARY_EXCEPTION:
        throw std::runtime_error(context + ": " + amqp_error_string2(x.library_error));

      case AMQP_RESPONSE_SERVER_EXCEPTION:
        switch (x.reply.id)
        {
          case AMQP_CONNECTION_CLOSE_METHOD:
          {
            auto m = static_cast<amqp_connection_close_t const *>(x.reply.decoded);

            throw std::runtime_error(context + ": server connection error "
                                     + std::to_string(m->reply_code)
                                     + ", message: " + get_string(m->reply_text));
          }

          case AMQP_CHANNEL_CLOSE_METHOD:
          {
            auto m = static_cast<amqp_channel_close_t const *>(x.reply.decoded);

            throw std::runtime_error(context + ": server channel error "
                                     + std::to_string(m->reply_code)
                                     + ", message: " + get_string(m->reply_text));
          }

          default:
            throw std::runtime_error(context + ":  unknown server error, method id "
                                     + std::to_string( x.reply.id));
        }
    }
  }

  namespace utils
  {
    struct request
    {
      amqp_envelope_t envelope;
    };

    struct response
    {
      amqp_rpc_reply_t res;
    };

    struct service
    {
      typedef request request_type;
      typedef response response_type;

      template <typename P, typename T>
      auto
      deserialize(request_type * r, P /*procedure*/, T & res) const
      {
        auto message = get_string(r->envelope.message.body);
        auto routing_key = get_string(r->envelope.routing_key);
        auto content_type = get_string(r->envelope.message.properties.content_type);
        auto exchange = get_string(r->envelope.exchange);

        //std::cout << "Delivery " << (unsigned) r->envelope.delivery_tag << " "
        //          << "exchange " << exchange << " "
        //          << "routingkey " << routing_key << " "
        //          << std::endl;

        if (r->envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG &&
            content_type == "application/json")
        {
          //std::cout << "Content-type: " << content_type << std::endl;
          //std::cout << "Message: " << message << std::endl;

          iod::json_decode<typename P::route_type::parameters_type>(res, message);
        }
        else
        {
          throw std::runtime_error("wrong content type: " + content_type);
        }
      }

      template <typename T>
      auto
      serialize(response_type * /*r*/, T const & /*res*/) const
      {
      }
    };

    struct socket
    {
      amqp_socket_t * socket = nullptr;
      amqp_connection_state_t conn;
    };

    struct tcp_socket:
      public socket
    {
      template <typename... O>
      tcp_socket(std::string const & host, unsigned short port, O &&... opts)
      {
        auto options = D(opts...);

        unsigned int r = options.get(s::_retry, 5);

        conn = amqp_new_connection();
        socket = amqp_tcp_socket_new(conn);
        if (!socket)
          throw std::runtime_error("amqp.tcp.socket.new");

        if (!retry("amqp.socket.open", [&]()
                   {
                     return amqp_socket_open(socket, host.c_str(), port) == 0;
                   }, r, 1s))
          throw std::runtime_error("amqp.socket.open");
      }

      //~tcp_socket()
      //{
      //  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
      //                    "amqp.connection.close");
      //  die_on_error(amqp_destroy_connection(conn), "amqp.destroy.connection");
      //}
    };
  }

  namespace context
  {
    template <typename S>
    struct basic
    {
      template <typename... O>
        basic( std::string const & host, unsigned short port, O &&... opts):
          socket(host, port, opts...)
      {
        auto options = D(opts...);
        auto username = options.username;
        auto password = options.password;

        // login
        die_on_amqp_error(amqp_login(socket.conn, "/", 0, 131072, 0,
                                     AMQP_SASL_METHOD_PLAIN,
                                     username.c_str(), password.c_str()),
                          "amqp.login");

        // open channel
        amqp_channel_open(socket.conn, 1);
        die_on_amqp_error(amqp_get_rpc_reply(socket.conn),
                          "amqp.channel.open");
      }

      S socket;
    };

    template <typename S>
    struct consumer:
      public basic<S>
    {
      template <typename A, typename... O>
      consumer(A const & api, std::string const & host, unsigned short port, O &&... opts):
        basic<S>(host, port, opts...)
      {
        auto options = D(opts...);

        no_ack = options.get(s::_no_ack, false);

        foreach(api) | [&] (auto& m)
        {
          iod::static_if<is_tuple<decltype(m.content)>::value>(
            [&] (auto /*m*/)
            {
              // If sio, recursion.
              throw std::runtime_error("FIXME: m.content is a tuple, not handle today");
            },
            [&] (auto m)
            {
              // Else, register the procedure.
              amqp_channel_t chan = 1;

              // declare queue
              auto r = amqp_queue_declare(this->socket.conn, chan,
                                          amqp_cstring_bytes(m.route.queue_name_as_string().c_str()),
                                          options.get(s::_passive, false),
                                          options.get(s::_durable, false),
                                          options.get(s::_exclusive, false),
                                          options.get(s::_auto_delete, false),
                                          amqp_empty_table);

              die_on_amqp_error(amqp_get_rpc_reply(this->socket.conn), "amqp.queue.declare");
              auto queuename = amqp_bytes_malloc_dup(r->queue);

              if (queuename.bytes == NULL)
                throw std::runtime_error("out of memory");

              // bind queue
              amqp_queue_bind(this->socket.conn, chan,
                              queuename,                                                  // Queue name
                              amqp_cstring_bytes(m.route.exchange_as_string()),           // Exchange name
                              amqp_cstring_bytes(m.route.routing_key_as_string().c_str()),// Routing key
                              amqp_empty_table);
              die_on_amqp_error(amqp_get_rpc_reply(this->socket.conn), "amqp.queue.bind");

              if (options.has(s::_prefetch_count))
              {
                // set prefetch count
                amqp_basic_qos(this->socket.conn, chan, 0, options.get(s::_prefetch_count, 1), false);
                die_on_amqp_error(amqp_get_rpc_reply(this->socket.conn), "amqp.basic.qos");
              }

              // register to consume
              amqp_basic_consume(this->socket.conn, chan,
                                 queuename,
                                 amqp_empty_bytes,
                                 options.get(s::_no_local, false),
                                 options.get(s::_no_ack, false),
                                 options.get(s::_exclusive, false),
                                 amqp_empty_table);
              die_on_amqp_error(amqp_get_rpc_reply(this->socket.conn), "amqp.basic.consume");

            },
            m);
        };
      }

      template <typename Service>
      auto run(Service & s)
      {
        while (1)
        {
          amqp_frame_t frame;

          utils::request rq;
          utils::response resp;

          amqp_maybe_release_buffers(basic<S>::socket.conn);

          resp.res = amqp_consume_message(basic<S>::socket.conn, &rq.envelope, NULL, 0);

          if (AMQP_RESPONSE_NORMAL != resp.res.reply_type)
          {
            if (AMQP_RESPONSE_LIBRARY_EXCEPTION == resp.res.reply_type &&
                AMQP_STATUS_UNEXPECTED_STATE == resp.res.library_error)
            {
              if (AMQP_STATUS_OK != amqp_simple_wait_frame(basic<S>::socket.conn, &frame))
              {
                return 1;
              }

              if (AMQP_FRAME_METHOD == frame.frame_type)
              {
                switch (frame.payload.method.id)
                {
                  case AMQP_BASIC_ACK_METHOD:
                    /* if we've turned publisher confirms on, and we've published a
                     * message here is a message being confirmed.
                     */
                    break;
                  case AMQP_BASIC_RETURN_METHOD:
                    /* if a published message couldn't be routed and the mandatory
                     * flag was set this is what would be returned. The message then
                     * needs to be read.
                     */
                    {
                      amqp_message_t message;
                      resp.res = amqp_read_message(basic<S>::socket.conn, frame.channel, &message, 0);
                      if (AMQP_RESPONSE_NORMAL != resp.res.reply_type)
                      {
                        return 1;
                      }

                      amqp_destroy_message(&message);
                      break;
                    }


                  case AMQP_CHANNEL_CLOSE_METHOD:
                    /* a channel.close method happens when a channel exception occurs,
                     * this can happen by publishing to an exchange that doesn't exist
                     * for example.
                     *
                     * In this case you would need to open another channel redeclare
                     * any queues that were declared auto-delete, and restart any
                     * consumers that were attached to the previous channel.
                     */
                    return 1;

                  case AMQP_CONNECTION_CLOSE_METHOD:
                    /* a connection.close method happens when a connection exception
                     * occurs, this can happen by trying to use a channel that isn't
                     * open for example.
                     *
                     * In this case the whole connection must be restarted.
                     */
                    return 1;

                  default:
                    std::cerr << "An unexpected method was received "
                              << frame.payload.method.id
                              << std::endl;
                    return 1;
                }
              }
            }

          }
          else
          {
            auto routing_key = get_string(rq.envelope.routing_key);
            auto exchange = get_string(rq.envelope.exchange);
            auto local_no_ack = no_ack;

            try
            {
              std::stringstream ss;

              ss << exchange << ": " << routing_key;

              s(ss.str(), &rq, &resp, *this);
            }
            catch(error::error const & e)
            {
              std::cerr << "Exception: " << e.status() << " " << e.what() << std::endl;
              local_no_ack = false;
            }
            catch(std::runtime_error const & e)
            {
              std::cerr << "Exception: " << e.what() << std::endl;
              local_no_ack = false;
            }

            if (!local_no_ack)
              die_on_error(amqp_basic_ack(basic<S>::socket.conn, rq.envelope.channel,
                                          rq.envelope.delivery_tag, 0),
                           "amqp.basic.ack");

            amqp_destroy_envelope(&rq.envelope);
          }
        }

        die_on_amqp_error(amqp_channel_close(basic<S>::socket.conn, 1, AMQP_REPLY_SUCCESS),
                          "amqp.channel.close");
        die_on_amqp_error(amqp_connection_close(basic<S>::socket.conn, AMQP_REPLY_SUCCESS),
                          "amqp.connection.close");
        die_on_error(amqp_destroy_connection(basic<S>::socket.conn),
                     "amqp.connection.destroy");

        return 0;
      }

      bool no_ack;
    };
  }

  template <typename C, typename A, typename M, typename... O>
  auto
  make_context(A const & api, M const & mf, std::string const & host, unsigned short port, O &&... opts)
  {
    auto ctx = C(api, host, port, opts...);
    auto m2 = std::tuple_cat(std::make_tuple(), mf);

    using service_t = service<utils::service, decltype(m2), utils::request*, utils::response*, C>;
    auto s = service_t(api, m2);

    return ctx.run(s);
  }

  template <typename S, typename A, typename M, typename... O>
  auto
  consume(A const & api, M const & mf, std::string const & host, unsigned short port, O &&... opts)
  {
    return make_context<context::consumer<S>>(api, mf, host, port, opts...);
  }

  template <typename S, typename A, typename... O>
  auto
  consume(A const & api, std::string const & host, unsigned short port, O &&... opts)
  {
    return make_context<context::consumer<S>>(api, std::make_tuple(), host, port, opts...);
  }
}
}
