#pragma once

#include <cstddef>

namespace Config {
constexpr int PORT = 80;
constexpr int LISTEN_BACKLOG = 10;
constexpr size_t REQUEST_BUFFER_SIZE = 65536;
constexpr size_t FILE_BUFFER_SIZE = 8192;
constexpr size_t ARCHIVE_BLOCK_SIZE = 10240;
constexpr const char* PATH_HOME_BASE = "/home/";
constexpr const char* PATH_WORKSPACE = "/workspace";
constexpr const char* PATH_ARCHIVE = "/workspace.tgz";
}  // namespace Config
