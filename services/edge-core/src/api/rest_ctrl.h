#pragma once
#include <drogon/HttpController.h>

namespace ctm {
namespace api {

class RestCtrl : public drogon::HttpController<RestCtrl, false> {
public:
    METHOD_LIST_BEGIN
    METHOD_ADD(RestCtrl::ingestTelemetry, "/api/v1/telemetry", drogon::Post);
    METHOD_ADD(RestCtrl::simulatorControl, "/api/v1/simulator/control", drogon::Post);
    METHOD_LIST_END

    void ingestTelemetry(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);
                         
    void simulatorControl(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace api
} // namespace ctm
