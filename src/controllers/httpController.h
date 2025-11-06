#pragma once

#include <string>

void handleRequest(int client);
void sendResponse(int socket, int status, const std::string& body);
