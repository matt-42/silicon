#pragma once

namespace sl
{

  struct tracking_cookie
  {
    tracking_cookie(std::string id) : id_(id) {}

    std::string& id() { return id_; }
    std::string id_;
  };


  std::string generate_tracking_id()
  {
    std::ostringstream os;
    std::random_device rd;
    os << std::hex << rd() << rd() << rd() << rd();
    return os.str();
  }

}
