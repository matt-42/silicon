#pragma once

namespace sl
{

  struct cookie_token
  {
    cookie_token(std::string token) : t_(token) {}

    std::string& token() { return t_; }
    std::string t_;
  };

}
