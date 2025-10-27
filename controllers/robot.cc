#include "robot.h"

#include <filesystem>
#include <fstream>

#include "../services/WorkspaceService.h"

static std::string workspacePath = "/home/default/";

void api::v1::Robot::info(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  Json::Value ret;
  ret["deviceSerial"] = "TMP0123";
  ret["result"] = 0;
  ret["result_msg"] = "";

  auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
  callback(resp);
}

void api::v1::Robot::running(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  Json::Value ret;
  ret["data"] = false;
  ret["result"] = 0;
  ret["result_msg"] = "";

  auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
  callback(resp);
}

void api::v1::Robot::workspaceImport(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  drogon::MultiPartParser fileUpload;
  if (fileUpload.parse(req) != 0) {
    Json::Value error;
    error["error"] = "Failed to parse multipart data";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k400BadRequest);
    callback(resp);
    return;
  }

  auto files = fileUpload.getFiles();
  if (files.empty()) {
    Json::Value error;
    error["error"] = "No file uploaded";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k400BadRequest);
    callback(resp);
    return;
  }

  auto& file = files[0];
  std::string originalFilename = file.getFileName();

  // Check if archive format is supported
  if (!services::WorkspaceService::isSupportedArchive(originalFilename)) {
    Json::Value error;
    error["error"] =
        "Unsupported file format. Supported: .zip, .tar, .tar.gz, .tgz, etc.";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k400BadRequest);
    callback(resp);
    return;
  }

  // If workspace already exists, remove it
  if (std::filesystem::exists(workspacePath)) {
    LOG_INFO << "Workspace already exists, removing: " << workspacePath;
    try {
      std::filesystem::remove_all(workspacePath);
    } catch (const std::exception& e) {
      LOG_ERROR << "Failed to remove existing workspace: " << e.what();
      Json::Value error;
      error["error"] = "Failed to remove existing workspace";
      error["message"] = e.what();
      auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
      resp->setStatusCode(drogon::k500InternalServerError);
      callback(resp);
      return;
    }
  }

  try {
    // Create temporary directory for uploaded file
    std::string tempDir = "/tmp/drogon-upload/";
    if (!std::filesystem::exists(tempDir)) {
      std::filesystem::create_directories(tempDir);
    }

    std::string tempFilePath =
        tempDir + drogon::utils::getUuid() + "_" + originalFilename;

    // Save uploaded file
    file.saveAs(tempFilePath);
    LOG_INFO << "File saved to: " << tempFilePath;

    // Extract archive using WorkspaceService
    auto result = services::WorkspaceService::importWorkspace(tempFilePath,
                                                              workspacePath);

    // Remove temporary file
    std::filesystem::remove(tempFilePath);

    if (!result.success) {
      // Clean up workspace directory if extraction failed
      if (std::filesystem::exists(workspacePath)) {
        std::filesystem::remove_all(workspacePath);
      }

      Json::Value error;
      error["error"] = "Failed to import workspace";
      error["message"] = result.errorMessage;
      auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
      resp->setStatusCode(drogon::k500InternalServerError);
      callback(resp);
      return;
    }

    // Success response
    Json::Value response;
    response["success"] = true;
    response["message"] = "Workspace imported successfully";
    response["data"] = result.data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k201Created);
    callback(resp);

    LOG_INFO << "Workspace imported";

  } catch (const std::exception& e) {
    LOG_ERROR << "Exception during import: " << e.what();

    // Clean up
    if (std::filesystem::exists(workspacePath)) {
      std::filesystem::remove_all(workspacePath);
    }

    Json::Value error;
    error["error"] = "Internal server error";
    error["message"] = e.what();
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k500InternalServerError);
    callback(resp);
  }
}

void api::v1::Robot::workspaceExport(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  // Check if workspace exists
  if (!std::filesystem::exists(workspacePath) ||
      !std::filesystem::is_directory(workspacePath)) {
    Json::Value error;
    error["error"] = "Workspace not found";
    error["message"] = "Workspace does not exist";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k404NotFound);
    callback(resp);
    return;
  }

  try {
    // Create temporary directory for export
    std::string tempDir = "/tmp/drogon-export/";
    if (!std::filesystem::exists(tempDir)) {
      std::filesystem::create_directories(tempDir);
    }

    std::string workspaceName = "workspace";

    std::string outputFilename = workspaceName + ".tar.gz";
    std::string outputPath = tempDir + outputFilename;

    // Export workspace using WorkspaceService
    auto result =
        services::WorkspaceService::exportWorkspace(workspacePath, outputPath);

    if (!result.success) {
      Json::Value error;
      error["error"] = "Failed to export workspace";
      error["message"] = result.errorMessage;
      auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
      resp->setStatusCode(drogon::k500InternalServerError);
      callback(resp);
      return;
    }

    // Read the compressed file
    std::ifstream file(outputPath, std::ios::binary);
    if (!file) {
      Json::Value error;
      error["error"] = "Failed to read exported file";
      auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
      resp->setStatusCode(drogon::k500InternalServerError);
      callback(resp);
      return;
    }

    std::string fileContent((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    file.close();

    // Remove temporary file
    std::filesystem::remove(outputPath);

    // Return file as download
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setBody(fileContent);
    resp->setContentTypeCode(drogon::CT_APPLICATION_OCTET_STREAM);
    resp->addHeader("Content-Disposition",
                    "attachment; filename=\"" + outputFilename + "\"");
    resp->setStatusCode(drogon::k200OK);
    callback(resp);

    LOG_INFO << "Workspace exported";

  } catch (const std::exception& e) {
    LOG_ERROR << "Exception during export: " << e.what();

    Json::Value error;
    error["error"] = "Internal server error";
    error["message"] = e.what();
    auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
    resp->setStatusCode(drogon::k500InternalServerError);
    callback(resp);
  }
}