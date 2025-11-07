#pragma once

#include <string>

namespace controllers {

class HttpController {
 public:
  void handleRequest(int client);

 private:
  void sendResponse(int socket, int status, const std::string& body);
};

}  // namespace controllers
