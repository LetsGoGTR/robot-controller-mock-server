#pragma once

#include <drogon/HttpController.h>

namespace api {
namespace v1 {
class Robot : public drogon::HttpController<Robot> {
 public:
  METHOD_LIST_BEGIN
  // use METHOD_ADD to add your custom processing function here;
  METHOD_ADD(Robot::running, "/running", drogon::Get);
  METHOD_ADD(Robot::exportWorkspace, "/export", drogon::Post);
  METHOD_ADD(Robot::importWorkspace, "/import", drogon::Post);
  METHOD_LIST_END
  // your declaration of processing function maybe like this:
  void running(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void exportWorkspace(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void importWorkspace(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
}  // namespace v1
}  // namespace api
