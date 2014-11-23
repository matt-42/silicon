#pragma once

namespace iod
{
  struct file
  {
    file(const std::string& path) : path_(path) {}
    const std::string& path() const { return path_; }

  private:
    std::string path_;
  };
}
