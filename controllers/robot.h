#pragma once

#include <drogon/HttpController.h>

namespace api {
class Robot : public drogon::HttpController<Robot> {
 public:
  METHOD_LIST_BEGIN
  // use METHOD_ADD to add your custom processing function here;
  METHOD_ADD(Robot::info, "/info", drogon::Get);
  METHOD_ADD(Robot::running, "/running", drogon::Get);
  METHOD_ADD(Robot::workspaceExport, "/export", drogon::Get);
  METHOD_ADD(Robot::workspaceImport, "/import", drogon::Post);
  METHOD_LIST_END
  // your declaration of processing function maybe like this:
  void info(const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void running(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void workspaceExport(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void workspaceImport(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
}  // namespace api
