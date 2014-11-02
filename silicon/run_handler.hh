#pragma once

#include <iod/sio.hh>
#include <iod/callable_traits.hh>
#include <iod/tuple_embeds.hh>

#include <silicon/server.hh>

namespace iod
{

  template <typename C>
  struct handler;

  // Simple object handler.
  template <typename F>
  std::enable_if_t<not iod::is_callable<F>::value>
  run_handler(const handler<F>& handler,
              request& request, response& response)
  {
    response << handler.content_;
  }

  // ===============================
  // Helper to build the tuple of arguments that
  // run_handler will apply to the handler.
  // ===============================

  template <typename T>
  inline int make_handler_argument(T, request&, response&)
  {
    static_assert(!std::is_same<T, T>::value,
                  "One of your handlers take an unsupported argument. valid argument types are request&"
                  ", response&, decltype(iod::D(...)), and the external handler arguments. Note that the "
                  "external handler arguments have to be included before this file.");
    return 0;
  }

  template <typename... ARGS>
  inline sio<ARGS...> make_handler_argument(sio<ARGS...>* r,
                                            request& request, response& response)
  {
    sio<ARGS...> res;
    iod::json_decode(res, request.get_body_stream());
    return res;
  }

  inline request& make_handler_argument(request*,
                                        request& request, response& response)
  {
    return request;
  }

  inline response& make_handler_argument(response*,
                                        request& request, response& response)
  {
    return response;
  }

  template <typename... T, typename F>
  auto apply_handler_arguments(request& request_, response& response_,
                               F f, std::tuple<T...>* params)
  {
    return f(make_handler_argument((std::remove_reference_t<T>*) 0, request_, response_)...);
  }


  // // Callable handlers.
  template <typename F>
  std::enable_if_t<iod::is_callable<F>::value>
  run_handler(const handler<F>& handler,
              request& request_, response& response_)
  {
    iod::static_if<callable_traits<F>::arity == 0>(
      [&] (auto& handler) {
        // No argument. Send the result of the handler.
        response_ << handler.content_();
      },
      [&] (auto& handler) {
        // make argument tuple.
        typedef iod::callable_arguments_tuple_t<F> T;

        iod::static_if<tuple_embeds<T, response&>::value>( // If handler take response as argument.

          // handlers taking response as argument are responsible of writing the response, 
          [&] (auto& handler, auto& request_, auto& response_) {
            apply_handler_arguments(request_, response_, handler.content_, (T*)0);
          },

          // handlers not taking response as argument return the response.
          [&] (auto& handler, auto& request_, auto& response_) {
            response_ << apply_handler_arguments(request_, response_, handler.content_, (T*)0);
          },
          handler, request_, response_);
      }, handler
      );
  }

}
