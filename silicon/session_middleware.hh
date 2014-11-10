#pragma once

namespace iod
{

  // Represents a session.
  struct session_middleware
  {
    void signin(const std::string& user,
                const std::string& password);
    void signout();
  };

  // Returns the session object.
  inline session_middleware make_handler_argument(session_middleware*,
                                                  request& request, response& response)
  {
    session_middleware the_new_session;
    // Build the session
    return the_new_session;
  }

}
