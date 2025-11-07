#include "utils.h"

namespace utils {

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

std::string jsonMsg(bool ok, const std::string& msg) {
  if (ok) {
    return R"({"success":true,"message":")" + msg + R"("})";
  }
  return R"({"success":false,"message":")" + msg + R"("})";
}

}  // namespace utils
