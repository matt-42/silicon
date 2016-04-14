#pragma once

# include <stdexcept>
# include <iostream>

# include <amqp_tcp_socket.h>
# include <amqp.h>
# include <amqp_framing.h>

# include <iod/json.hh>

# include <silicon/symbols.hh>
# include <silicon/service.hh>

namespace sl
{
namespace rmq
{
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

						throw std::runtime_error(context + ": server connection error " + std::to_string(m->reply_code)
								+ ", message: " + get_string(m->reply_text));
					}

					case AMQP_CHANNEL_CLOSE_METHOD:
					{
						auto m = static_cast<amqp_channel_close_t const *>(x.reply.decoded);

						throw std::runtime_error(context + ": server channel error " + std::to_string(m->reply_code)
								+ ", message: " + get_string(m->reply_text));
					}

					default:
						throw std::runtime_error(context + ":  unknown server error, method id " + std::to_string( x.reply.id));
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
			deserialize(request_type * r, P procedure, T & res) const
			{

				auto message = get_string(r->envelope.message.body);
				//auto routing_key = get_string(r->envelope.routing_key);
				//auto content_type = get_string(r->envelope.message.properties.content_type);
				//auto exchange = get_string(r->envelope.exchange);

				//std::cout << "Delivery " << (unsigned) r->envelope.delivery_tag << " "
				//		  << "exchange " << exchange << " "
				//		  << "routingkey " << routing_key << " "
				//		  << std::endl;

				if (r->envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG)
				{
					//std::cout << "Content-type: " << content_type << std::endl;
					//std::cout << "Message: " << message << std::endl;

					iod::json_decode<typename P::route_type::parameters_type>(res, message);
				}
				//std::cout << "----" << std::endl;
			}

			template <typename T>
			auto
			serialize(response_type * r, T const & res) const
			{
			}
		};
	};

	namespace context
	{
		struct basic
		{
			template <typename A, typename... O>
			basic(A const & api, unsigned short port, O &&... opts)
			{
				auto options = D(opts...);
				auto hostname = options.hostname;
				auto username = options.username;
				auto password = options.password;

				conn = amqp_new_connection();
				socket = amqp_tcp_socket_new(conn);

				if (!socket)
					throw std::runtime_error("creating TCP socket");

				auto status = amqp_socket_open(socket, hostname.c_str(), port);
				if (status)
					throw std::runtime_error("opening TCP socket");

				die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, username.c_str(), password.c_str()), "Loggin in");
				amqp_channel_open(conn, 1);
				die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
			}

			amqp_socket_t * socket = nullptr;
			amqp_connection_state_t conn;
		};

		struct worker:
			public basic
		{
			template <typename A, typename... O>
			worker(A const & api, unsigned short port, O &&... opts):
				basic(api, port, opts...)
			{
			}

			template <typename S>
			auto run(S & s)
			{
				return 0;
			}
		};

		struct consumer:
			public basic
		{
			template <typename A, typename... O>
			consumer(A const & api, unsigned short port, O &&... opts):
				basic(api, port, opts...)
			{
				foreach(api) | [&] (auto& m)
				{
					iod::static_if<is_tuple<decltype(m.content)>::value>(
							[&] (auto m) { // If sio, recursion.
								throw std::runtime_error("FIXME: m.content is a tuple, not handle today");
							},
							[&] (auto m) { // Else, register the procedure.
								amqp_queue_declare_ok_t * r = amqp_queue_declare(conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
								amqp_bytes_t queuename;

								die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
								queuename = amqp_bytes_malloc_dup(r->queue);
								if (queuename.bytes == NULL)
								{
									throw std::runtime_error("Out of memory while copying queue name");
								}

								amqp_queue_bind(conn, 1, queuename,
												amqp_cstring_bytes(m.route.verb_as_string()),
												amqp_cstring_bytes(m.route.path_as_string(false).c_str()),
												amqp_empty_table);
								die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");

								amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
								die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

								queuenames.emplace_back(queuename);
							},
					m);
				};
			}

			template <typename S>
			auto run(S & s)
			{
				while (1)
				{
					utils::request rq;
					utils::response resp;

					amqp_maybe_release_buffers(conn);

					resp.res = amqp_consume_message(conn, &rq.envelope, NULL, 0);

					if (AMQP_RESPONSE_NORMAL != resp.res.reply_type)
						break;

					auto routing_key = get_string(rq.envelope.routing_key);
					auto exchange = get_string(rq.envelope.exchange);

					try
					{
						s(exchange + routing_key, &rq, &resp, *this);
					}
					catch(const error::error& e)
					{
						std::cerr << "Exception: " << e.status() << " " << e.what() << std::endl;
					}
					catch(const std::runtime_error& e)
					{
						std::cerr << "Exception: " << e.what() << std::endl;
					}

					amqp_destroy_envelope(&rq.envelope);
				}

				die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
				die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
				die_on_error(amqp_destroy_connection(conn), "Ending connection");

				return 0;
			}

			std::vector<amqp_bytes_t> queuenames;
		};
	};

	template <typename C, typename A, typename M, typename... O>
	auto
	make_context(A const & api, M const & mf, unsigned short port, O &&... opts)
	{
		auto ctx = C(api, port, opts...);
		auto m2 = std::tuple_cat(std::make_tuple(), mf);

		using service_t = service<utils::service, decltype(m2), utils::request*, utils::response*, C>;
		auto s = service_t(api, m2);

		return ctx.run(s);
	}

	template <typename A, typename M, typename... O>
	auto
	consume(A const & api, M const & mf, unsigned short port, O &&... opts)
	{
		return make_context<context::consumer>(api, mf, port, opts...);
	}

	template <typename A, typename... O>
	auto
	consume(A const & api, unsigned short port, O &&... opts)
	{
		return make_context<context::consumer>(api, std::make_tuple(), port, opts...);
	}

	template <typename A, typename M, typename... O>
	auto
	work(A const & api, M const & mf, unsigned short port, O &&... opts)
	{
		return make_context<context::worker>(api, mf, port, opts...);
	}

	template <typename A, typename... O>
	auto
	work(A const & api, unsigned short port, O &&... opts)
	{
		return make_context<context::worker>(api, std::make_tuple(), port, opts...);
	}
};
};
