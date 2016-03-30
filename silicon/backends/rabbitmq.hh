#pragma once

# include <stdexcept>
# include <iostream>

# include <amqp_tcp_socket.h>
# include <amqp.h>
# include <amqp_framing.h>

# include <iod/json.hh>

# include <silicon/symbols.hh>
# include <silicon/service.hh>

iod_define_symbol(hostname)
iod_define_symbol(exchange)
iod_define_symbol(bindingkey)
iod_define_symbol(username)
iod_define_symbol(password)

namespace sl
{
	struct rmq_request
	{
		amqp_envelope_t				envelope;
	};

	struct rmq_response
	{
		amqp_rpc_reply_t			res;
	};

	struct rmq_service_utils
	{
		typedef rmq_request				request_type;
		typedef rmq_response			response_type;

		template <typename P, typename T>
		auto
		deserialize(request_type *		r,
					P					procedure,
					T &					res) const
		{
			auto						routing_key =  std::string(static_cast<char const *>(r->envelope.routing_key.bytes), r->envelope.routing_key.len);
			auto						message = std::string(static_cast<char const *>(r->envelope.message.body.bytes), r->envelope.message.body.len);

			std::cout << "Delivery " << (unsigned) r->envelope.delivery_tag << " "
					  << "exchange " << std::string(static_cast<char const *>(r->envelope.exchange.bytes), r->envelope.exchange.len) << " "
					  << "routingkey " << routing_key << " "
					  << std::endl;

			if (r->envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG)
			{
				std::cout << "Content-type: " << std::string(static_cast<char const *>(r->envelope.message.properties.content_type.bytes), r->envelope.message.properties.content_type.len) << std::endl;
				std::cout << "Message: " << std::string(static_cast<char const *>(r->envelope.message.body.bytes), r->envelope.message.body.len) << std::endl;
			}
			std::cout << "----" << std::endl;

			typename P::route_type::get_parameters_type u2;
			iod::json_decode(res, message);
		}

		template <typename T>
		auto
		serialize(response_type *		r,
				  T const &				res) const
		{
		}
	};

	struct rmq_context
	{
		template <typename... O>
		rmq_context(unsigned short		port,
				    O &&...				opts)
		{
			auto						options		= D(opts...);
			auto						hostname	= options.get(s::_hostname,		"localhost"); 
			auto						username	= options.get(s::_username,		"guest"); 
			auto						password	= options.get(s::_password,		"guest"); 

			conn	= amqp_new_connection();
			socket	= amqp_tcp_socket_new(conn);

			if (!socket)
				throw std::runtime_error("creating TCP socket");

			status = amqp_socket_open(socket, hostname.c_str(), port);
			if (status)
				throw std::runtime_error("opening TCP socket");

			amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, username.c_str(), password.c_str());
			amqp_channel_open(conn, 1);
			amqp_get_rpc_reply(conn);
		}

		int								status;
		amqp_socket_t *					socket = nullptr;
		amqp_connection_state_t			conn;
		std::vector<amqp_bytes_t>		queuenames;
	};

	template <typename A, typename M, typename... O>
	auto
	make_rmq_context(A const &			api,
					 M const &			mf,
					 unsigned short		port,
					 O &&...			opts)
	{
		auto							ctx			= rmq_context(port, opts...);
		auto							options		= D(opts...);
		auto							exchange	= options.get(s::_exchange,		"");

		foreach(api) | [&] (auto& m)
		{
			iod::static_if<is_tuple<decltype(m.content)>::value>(
					[&] (auto m) { // If sio, recursion.
						throw std::runtime_error("FIXME: m.content is a tuple, not handle today");
					},
					[&] (auto m) { // Else, register the procedure.
						std::stringstream bindingkey;

						foreach(m.route.path) | [&] (auto e)
						{

							iod::static_if<is_symbol<decltype(e)>::value>(
									[&] (auto e2) {
										bindingkey << std::string("/") + e2.name();
									},
									[&] (auto e2) {
										// FIXME: dynamic symbol
									},
							e);
						};

						amqp_queue_declare_ok_t *	r = amqp_queue_declare(ctx.conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
						amqp_bytes_t				queuename;

						amqp_get_rpc_reply(ctx.conn);
						queuename = amqp_bytes_malloc_dup(r->queue);
						if (queuename.bytes == NULL)
						{
							throw std::runtime_error("Out of memory while copying queue name");
						}

						amqp_queue_bind(ctx.conn, 1, queuename, amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(bindingkey.str().c_str()), amqp_empty_table);
						amqp_get_rpc_reply(ctx.conn);

						amqp_basic_consume(ctx.conn, 1, queuename, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
						amqp_get_rpc_reply(ctx.conn);

						ctx.queuenames.emplace_back(queuename);
					},
			m);
		};

		return ctx;
	}

	template <typename A, typename M, typename... O>
	auto
	rmq_serve(A const &					api,
			  M	const &					mf,
			  unsigned short			port,
			  O &&...					opts)
	{
		auto ctx = make_rmq_context(api, mf, port, opts...);

		auto m2 = std::tuple_cat(std::make_tuple(), mf);
		using service_t = service<rmq_service_utils,
								  decltype(m2),
								  rmq_request*, rmq_response*,
								  decltype(ctx)>;
		auto s = service_t(api, m2);
 
		while (1)
		{
			rmq_request					rq;
			rmq_response				resp;

			amqp_maybe_release_buffers(ctx.conn);

			resp.res = amqp_consume_message(ctx.conn, &rq.envelope, NULL, 0);

			if (AMQP_RESPONSE_NORMAL != resp.res.reply_type)
				break;

			auto						routing_key  =  std::string(static_cast<char const *>(rq.envelope.routing_key.bytes), rq.envelope.routing_key.len);

			try
			{
				// FIXME: should get the prefix from the type of the current envelope.
				s("/*" + routing_key, &rq, &resp, ctx);
			}
			catch(const error::error& e)
			{
				std::cerr << "Exception: " << e.status() << " " << e.what() << std::endl;
				//resp.status = e.status();
				//std::string m = e.what();
				//resp.body = m.data();
			}
			catch(const std::runtime_error& e)
			{
				std::cerr << "Exception: " << e.what() << std::endl;
				//resp.status = 500;
				//resp.body = "Internal server error.";
			}

			//amqp_dump(envelope.message.body.bytes, envelope.message.body.len);

			amqp_destroy_envelope(&rq.envelope);
		}
		return 0;
	}

	template <typename A, typename... O>
	auto
	rmq_serve(A const &					api,
			  unsigned short			port,
			  O &&...					opts)
	{
		return rmq_serve(api, std::make_tuple(), port, opts...);
	}
};
