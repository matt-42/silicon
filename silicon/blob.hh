#pragma once

namespace sl
{

  struct blob : public std::string
  {
    blob() : std::string() {}
    blob(const std::string& s) : std::string(s) {}
    blob(const blob& b) : std::string(b) {}
  };

}
