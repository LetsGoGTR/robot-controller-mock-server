#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>

#include "controllers/httpController.h"
#include "utils/config.h"

int main() {
  try {
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
      throw std::runtime_error("Socket failed");
    }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(Config::PORT);

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) < 0) {
      throw std::runtime_error("Bind failed");
    }
    if (listen(server, Config::LISTEN_BACKLOG) < 0) {
      throw std::runtime_error("Listen failed");
    }

    std::cout << "Server started on port " << Config::PORT << '\n';

    controllers::HttpController httpController;

    while (true) {
      try {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client = accept(server, (sockaddr*)&client_addr, &len);
        if (client < 0) {
          continue;
        }

        httpController.handleRequest(client);
        close(client);
      } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
      }
    }

    close(server);
  } catch (const std::exception& e) {
    std::cerr << "Fatal: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
