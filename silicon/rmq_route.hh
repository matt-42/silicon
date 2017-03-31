#pragma once

namespace sl
{
namespace rmq
{
	template <typename P>
	struct params_t
	{
		params_t(P p) : params(p) {}
		P params;
	};

	template <typename... P>
	auto parameters(P... p) {
		typedef decltype(D(std::declval<P>()...)) sio_type;
		return params_t<sio_type>(D(p...));
	}

	template <typename T>
	struct verb: public T, public iod::assignable<verb<T>>, public iod::Exp<verb<T>>
	{
		using iod::assignable<verb<T>>::operator=;
	};

	struct verb_direct { char const * to_string() { return "amq.direct"; } }; static verb<verb_direct> RMQ_DIRECT;

	template <typename V = verb_direct,
			  typename S = std::tuple<>,
			  typename P = iod::sio<>>
	struct route
	{
		typedef S path_type;
		typedef P parameters_type;

		route() {}
		route(P p1): params(p1) {}

		template <typename NS, typename NP>
		auto route_builder(NS s, NP p) const
		{
			return route<V, NS, NP>(p);
		}


		template <typename T>
		auto path_append(iod::symbol<T> const & s) const
		{
			auto new_path = std::tuple_cat(path, std::make_tuple(T()));
			return route_builder(new_path, params);
		}

		template <typename NP>
		auto format_params(NP const & params) const
		{
			// Forward default values and set default string parameters.
			auto params2 = foreach(params.params) | [&] (auto& m) {
				return static_if<!iod::is_symbol<decltype(m)>::value>(
					[&] (auto x) {
						return x.symbol() = x.value();
					},
					[] (auto x) {
						return x = std::string();
					}, m);
			};

			// Set the optional tag for the iod deserializer and default values.
			return foreach(params2) | [&] (auto& m) {
				return static_if<is_optional<decltype(m.value())>::value>(
					[&] (auto x) {
						return x.symbol()(_optional) = x.value().value;
					},
					[] (auto x) {
						return x.symbol() = x.value();
					}, m);
			};

		}

		// Set get params.
		template <typename NP>
		auto set_params(params_t<NP> const & new_params) const
		{
			return route_builder(path, format_params(new_params));
		}

		auto all_params() const
		{
			return params;
		}

		auto verb_as_string(verb_direct v) const { return v.to_string(); }
		auto verb_as_string() const { return verb_as_string(verb); }

		std::string path_as_string(bool with_verb = true) const
		{
			std::string s;

			if (with_verb)
				s += verb_as_string(verb);
			foreach(path) | [&] (auto e)
			{
				static_if<is_symbol<decltype(e)>::value>(
						[&] (auto e2) { s += std::string("/") + e2.name(); },
						[&] (auto) { s += std::string("/*"); }, e);
			};
			return s;
		}

		std::string string_id() const
		{
			return path_as_string();
		}

		V verb;
		S path;
		P params;
	};

	namespace internal
	{
		template <typename B, typename S>
		auto make_route(B b, iod::symbol<S> const & s)
		{
			return b.path_append(s);
		}

		template <typename B, typename V>
		auto make_route(B b, verb<V> const & v)
		{
			return b.set_verb(v);
		}

		template <typename B, typename P>
		auto make_route(B b, const params_t<P>& new_params)
		{
			return b.set_params(new_params);
		}

		template <typename B, typename S, typename V>
		auto make_route(B b, iod::array_subscript_exp<S, V> const & v)
		{
			return b.path_append(v);
		}

		template <typename B, typename L, typename R>
		auto make_route(B b, iod::div_exp<L, R> const & r);

		template <typename B, typename L, typename R>
		auto make_route(B b, iod::mult_exp<L, R> const & r)
		{
			return make_route(make_route(b, r.lhs), r.rhs);
		}

		template <typename B, typename L, typename R>
		auto make_route(B b, iod::div_exp<L, R> const & r)
		{
			return make_route(make_route(b, r.lhs), r.rhs);
		}
	};

};
	template <typename... R1, typename... R2>
	auto route_cat(rmq::route<R1...> const & r1,
				   rmq::route<R2...> const & r2)
	{
		return r1.append(r2);
	}

	template <typename... R1, typename R2>
	auto route_cat(rmq::route<R1...> const & r1,
				   R2 const & r2)
	{
		return rmq::internal::make_route(r1, r2);
	}

	template <typename V, typename S, typename P>
	auto make_route(const rmq::route<V, S, P>& r)
	{
		return r;
	}

	template <typename E>
	auto make_route(const E& exp)
	{
		rmq::route<> b;
		return rmq::internal::make_route(b, exp);
	}
};
