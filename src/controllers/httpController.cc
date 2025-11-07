#include "httpController.h"

#include <sys/socket.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <sstream>

#include "../services/workspaceService.h"
#include "../utils/config.h"
#include "../utils/utils.h"

namespace fs = std::filesystem;

namespace controllers {

void HttpController::sendResponse(int socket, int status,
                                  const std::string& body) {
  const char* text;

  if (status == 200) {
    text = "OK";
  } else if (status == 400) {
    text = "Bad Request";
  } else if (status == 404) {
    text = "Not Found";
  } else {
    text = "Internal Server Error";
  }

  std::string response = "HTTP/1.1 " + std::to_string(status) + " " + text +
                         "\r\n"
                         "Content-Type: application/json\r\n"
                         "Content-Length: " +
                         std::to_string(body.length()) + "\r\n\r\n" + body;

  send(socket, response.c_str(), response.length(), 0);
}

void HttpController::handleRequest(int client) {
  char buf[Config::REQUEST_BUFFER_SIZE] = {0};
  if (read(client, buf, sizeof(buf) - 1) <= 0) return;

  try {
    std::string req(buf);
    std::istringstream stream(req);
    std::string method, path, line;

    std::getline(stream, line);
    std::istringstream(line) >> method >> path;

    std::cout << method << " " << path << '\n';

    if (method != "POST") {
      sendResponse(client, 404, utils::jsonMsg(false, "Not found"));
      return;
    }

    while (std::getline(stream, line) && line != "\r" && !line.empty()) {
    }

    std::string body;
    std::getline(stream, body, '\0');

    std::string user = utils::extractJson(body, "user");

    if (user.empty()) {
      sendResponse(client, 400, utils::jsonMsg(false, "Missing user"));
      return;
    }

    if (user.find("..") != std::string::npos ||
        user.find("/") != std::string::npos) {
      sendResponse(client, 400, utils::jsonMsg(false, "Invalid user"));
      return;
    }

    if (!fs::exists(Config::PATH_HOME_BASE + user)) {
      sendResponse(client, 404, utils::jsonMsg(false, "User not found"));
      return;
    }

    services::WorkspaceService workspaceService;

    if (path == "/compress") {
      workspaceService.compress(user);
      sendResponse(client, 200, utils::jsonMsg(true, "Compressed"));
    } else if (path == "/extract") {
      workspaceService.extract(user);
      sendResponse(client, 200, utils::jsonMsg(true, "Extracted"));
    } else {
      sendResponse(client, 404, utils::jsonMsg(false, "Not found"));
    }
  } catch (const std::exception& e) {
    sendResponse(client, 500,
                 R"({"error":")" + std::string(e.what()) + R"("})");
  }
}

}  // namespace controllers
