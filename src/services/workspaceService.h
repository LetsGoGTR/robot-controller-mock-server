#pragma once

#include <string>

namespace services {

class WorkspaceService {
 public:
  void compress(const std::string& user);
  void extract(const std::string& user);
};

}  // namespace services
