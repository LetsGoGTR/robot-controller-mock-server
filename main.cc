#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

std::string extractJson(const std::string& json, const std::string& key) {
  size_t pos = json.find("\"" + key + "\"");
  if (pos == std::string::npos) {
    return "";
  }
  pos = json.find(":", pos);
  pos = json.find("\"", pos);
  size_t end = json.find("\"", pos + 1);
  if (pos != std::string::npos && end != std::string::npos) {
    return json.substr(pos + 1, end - pos - 1);
  }
  return "";
}

void sendResponse(int socket, int status, const std::string& body) {
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

void addDirToArchive(archive* a, const std::string& path,
                     const std::string& prefix) {
  DIR* dir = opendir(path.c_str());
  if (!dir) {
    throw std::runtime_error("Cannot open directory");
  }

  try {
    struct dirent* ent;
    while ((ent = readdir(dir))) {
      std::string name = ent->d_name;
      if (name == "." || name == "..") {
        continue;
      }

      std::string full = path + "/" + name;
      std::string arch = prefix + "/" + name;
      struct stat st;
      if (stat(full.c_str(), &st) != 0) {
        continue;
      }

      archive_entry* entry = archive_entry_new();
      archive_entry_set_pathname(entry, arch.c_str());
      archive_entry_copy_stat(entry, &st);
      archive_write_header(a, entry);

      if (S_ISREG(st.st_mode)) {
        std::ifstream file(full, std::ios::binary);
        char buf[8192];
        while (file.read(buf, sizeof(buf)) || file.gcount() > 0) {
          archive_write_data(a, buf, file.gcount());
        }
      }

      archive_entry_free(entry);
      if (S_ISDIR(st.st_mode)) {
        addDirToArchive(a, full, arch);
      }
    }
    closedir(dir);
  } catch (...) {
    closedir(dir);
    throw;
  }
}

void compress(const std::string& user) {
  std::string workspace = "/home/" + user + "/workspace";
  std::string output = "/home/" + user + "/tmp/workspace.tgz";

  fs::create_directories("/home/" + user + "/tmp");
  fs::remove(output);

  archive* a = archive_write_new();
  if (!a) {
    throw std::runtime_error("Failed to create archive");
  }

  try {
    archive_write_add_filter_gzip(a);
    archive_write_set_format_pax_restricted(a);
    if (archive_write_open_filename(a, output.c_str()) != ARCHIVE_OK) {
      throw std::runtime_error("Failed to open output");
    }
    addDirToArchive(a, workspace, "workspace");
    archive_write_close(a);
    archive_write_free(a);
  } catch (...) {
    archive_write_free(a);
    throw;
  }
}

void extract(const std::string& user) {
  std::string workspace = "/home/" + user + "/workspace";
  std::string input = "/home/" + user + "/tmp/workspace.tgz";

  fs::remove_all(workspace);

  archive* a = archive_read_new();
  if (!a) {
    throw std::runtime_error("Failed to create reader");
  }

  try {
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    if (archive_read_open_filename(a, input.c_str(), 10240) != ARCHIVE_OK) {
      throw std::runtime_error("Failed to open archive");
    }

    archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
      std::string path = "/home/" + user + "/" + archive_entry_pathname(entry);
      archive_entry_set_pathname(entry, path.c_str());

      archive* ext = archive_write_disk_new();
      archive_write_disk_set_options(
          ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM);

      if (archive_write_header(ext, entry) == ARCHIVE_OK) {
        const void* buf;
        size_t size;
        int64_t offset;
        while (archive_read_data_block(a, &buf, &size, &offset) == ARCHIVE_OK) {
          archive_write_data_block(ext, buf, size, offset);
        }
      }
      archive_write_free(ext);
    }
    archive_read_close(a);
    archive_read_free(a);
  } catch (...) {
    archive_read_free(a);
    throw;
  }
}

void handleRequest(int client) {
  char buf[65536] = {0};
  if (read(client, buf, sizeof(buf) - 1) <= 0) {
    return;
  }

  try {
    std::string req(buf);
    std::istringstream stream(req);
    std::string method, path, line;

    std::getline(stream, line);
    std::istringstream(line) >> method >> path;

    std::cout << method << " " << path << '\n';

    if (method != "POST") {
      sendResponse(client, 404, R"({"success":false,"message":"Not found"})");

      return;
    }

    while (std::getline(stream, line) && line != "\r" && !line.empty()) {
    }

    std::string body;
    std::getline(stream, body, '\0');

    std::string user = extractJson(body, "user");
    if (user.empty()) {
      sendResponse(client, 400,
                   R"({"success":false,"message":"Missing user"})");
      return;
    }

    if (path == "/compress") {
      compress(user);
      sendResponse(client, 200, R"({"success":true,"message":"Compressed"})");
    } else if (path == "/extract") {
      extract(user);
      sendResponse(client, 200, R"({"success":true,"message":"Extracted"})");
    } else {
      sendResponse(client, 404, R"({"success":false,"message":"Not found"})");
    }
  } catch (const std::exception& e) {
    sendResponse(client, 500,
                 R"({"error":")" + std::string(e.what()) + R"("})");
  }
}

int main() {
  try {
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
      throw std::runtime_error("Socket failed");
    }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
               sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(80);

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) < 0) {
      throw std::runtime_error("Bind failed");
    }
    if (listen(server, 10) < 0) {
      throw std::runtime_error("Listen failed");
    }

    std::cout << "Server started on port 80\n";

    while (true) {
      try {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client = accept(server, (sockaddr*)&client_addr, &len);
        if (client < 0) {
          continue;
        }

        handleRequest(client);
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
