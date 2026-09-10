#include "rest_ctrl.h"
#include "websocket_ctrl.h"
#include "../ingestion/live_state_store.h"
#include "../rules/alarm_engine.h"
#include "../rules/correlation_engine.h"
#include "../rules/energy_engine.h"
#include <drogon/HttpClient.h>
#include <json/json.h>

namespace ctm {
namespace api {

void RestCtrl::ingestTelemetry(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto jsonObj = req->getJsonObject();
    if (!jsonObj) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    try {
        domain::TelemetryEvent event;
        event.asset_id = (*jsonObj)["asset_id"].asString();
        event.timestamp_ns = (*jsonObj)["timestamp_ns"].asUInt64();
        event.source = (*jsonObj)["source"].asString();
        
        auto tags = (*jsonObj)["tags"];
        for (auto it = tags.begin(); it != tags.end(); ++it) {
            event.tag_id = it.key().asString();
            event.value = it->asDouble();
            event.quality = domain::Quality::GOOD;
            
            // 1. Update live state
            ingestion::LiveStateStore::getInstance().updateTelemetry(event);
            
            // 2. Rules Evaluation
            rules::AlarmEngine::getInstance().evaluateTelemetry(event);
            rules::CorrelationEngine::getInstance().evaluateTelemetry(event);
            rules::EnergyEngine::getInstance().processTelemetry(event);
        }

        // 3. Broadcast raw telemetry via WS
        Json::Value wsMsg;
        wsMsg["type"] = "telemetry.update";
        wsMsg["timestamp"] = event.timestamp_ns;
        wsMsg["payload"] = *jsonObj;
        
        Json::FastWriter writer;
        WebSocketCtrl::broadcast(writer.write(wsMsg));

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        callback(resp);
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
    }
}

void RestCtrl::simulatorControl(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    // Forward command to the Python simulator listening on 8082
    auto client = drogon::HttpClient::newHttpClient("http://127.0.0.1:8082");
    auto simReq = drogon::HttpRequest::newHttpRequest();
    simReq->setMethod(drogon::Post);
    simReq->setPath("/control");
    if (req->getJsonObject()) {
        simReq->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        Json::FastWriter writer;
        simReq->setBody(writer.write(*req->getJsonObject()));
    }
    
    client->sendRequest(simReq, [callback](drogon::ReqResult result, const drogon::HttpResponsePtr& simResp) {
        if (result == drogon::ReqResult::Ok && simResp) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            callback(resp);
        } else {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k502BadGateway);
            callback(resp);
        }
    });
}

} // namespace api
} // namespace ctm
