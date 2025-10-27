#include "robot.h"

void api::v1::Robot::running(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  Json::Value ret;
  ret["data"] = true;
  ret["result"] = 0;
  ret["result_msg"] = "";

  auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
  callback(resp);
}

void api::v1::Robot::exportWorkspace(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {}

void api::v1::Robot::importWorkspace(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {}
