#pragma once

#include <string>

namespace utils {

std::string extractJson(const std::string& json, const std::string& key);
std::string jsonMsg(bool ok, const std::string& msg);

}  // namespace utils
