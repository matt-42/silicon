#pragma once

namespace sl
{

  struct blob : public std::string
  {
    using std::string::string;
    using std::string::operator=;

    blob() : std::string() {}
    // blob(const char* s) : std::string(s) {}
    // blob(const std::string& s) : std::string(s) {}
    // blob(const blob& b) : std::string(b) {}

    //operator=
  };

}
