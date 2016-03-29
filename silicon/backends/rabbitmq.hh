#pragma once

# include <stdexcept>
# include <iostream>

# include <amqp_tcp_socket.h>
# include <amqp.h>
# include <amqp_framing.h>

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
	};

	struct rmq_response
	{
	};

	struct rmq_service_utils
	{
		typedef rmq_request		request_type;
		typedef rmq_response	response_type;

		template <typename P, typename T>
		auto
		deserialize(request_type *		r,
					P					procedure,
					T &					res) const
		{
		}

		template <typename T>
		auto
		serialize(response_type *		r,
				  T const &				res) const
		{
		}
	};

	template <typename A, typename M, typename... O>
	auto
	rmq_serve(A const &					api,
			  M const &					middleware_factories,
			  unsigned short			port,
			  O &&...					opts)
	{
		int								status;
		amqp_socket_t *					socket = nullptr;
		amqp_connection_state_t			conn;
		amqp_bytes_t					queuename[2];
		auto							options = D(opts...);

		auto							hostname	= options.get(s::_hostname,		"localhost"); 
		auto							exchange	= options.get(s::_exchange,		""); 
		auto							bindingkey	= options.get(s::_bindingkey,	"test"); 
		auto							username	= options.get(s::_username,		"guest"); 
		auto							password	= options.get(s::_password,		"guest"); 

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

		for (int i = 0; i < 2; ++i)
		{
			amqp_queue_declare_ok_t *	r = amqp_queue_declare(conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);

			amqp_get_rpc_reply(conn);
			queuename[i] = amqp_bytes_malloc_dup(r->queue);
			if (queuename[i].bytes == NULL)
			{
				throw std::runtime_error("Out of memory while copying queue name");
			}
		}

		using service_t = service<rmq_service_utils,
								  decltype(middleware_factories),
								  rmq_request*, rmq_response*>;
		auto s = new service_t(api, middleware_factories);
 
		amqp_queue_bind(conn, 1, queuename[0], amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(bindingkey.c_str()), amqp_empty_table);
		amqp_get_rpc_reply(conn);

		amqp_basic_consume(conn, 1, queuename[0], amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
		amqp_get_rpc_reply(conn);

		amqp_queue_bind(conn, 1, queuename[1], amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes("lol"), amqp_empty_table);
		amqp_get_rpc_reply(conn);

		amqp_basic_consume(conn, 1, queuename[1], amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
		amqp_get_rpc_reply(conn);

		while (1)
		{
			amqp_rpc_reply_t			res;
			amqp_envelope_t				envelope;

			amqp_maybe_release_buffers(conn);

			res = amqp_consume_message(conn, &envelope, NULL, 0);

			if (AMQP_RESPONSE_NORMAL != res.reply_type)
				break;

			std::cout << "Delivery " << (unsigned) envelope.delivery_tag << std::endl;//", exchange %.*s routingkey %.*s\n",
			//printf("Delivery %u, exchange %.*s routingkey %.*s\n",
			//		(unsigned) envelope.delivery_tag,
			//		(int) envelope.exchange.len, (char *) envelope.exchange.bytes,
			//		(int) envelope.routing_key.len, (char *) envelope.routing_key.bytes);

			if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG)
			{
				//printf("Content-type: %.*s\n",
				//		(int) envelope.message.properties.content_type.len,
				//		(char *) envelope.message.properties.content_type.bytes);
			}
			printf("----\n");

			//amqp_dump(envelope.message.body.bytes, envelope.message.body.len);

			amqp_destroy_envelope(&envelope);
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
