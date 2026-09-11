#include "rest_ctrl.h"
#include "websocket_ctrl.h"
#include "../ingestion/live_state_store.h"
#include "../rules/alarm_engine.h"
#include "../rules/correlation_engine.h"
#include "../rules/energy_engine.h"
#include "../storage/sqlite_repository.h"
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

void RestCtrl::getContext(const drogon::HttpRequestPtr& /*req*/,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    Json::Value respJson;
    respJson["contextVersion"] = (Json::UInt64)ingestion::LiveStateStore::getInstance().getContextVersion();
    respJson["timestamp"] = (Json::UInt64)std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    auto assets = storage::SQLiteRepository::getInstance().getAllAssets();
    Json::Value assetsJson(Json::arrayValue);
    for (const auto& a : assets) {
        Json::Value aj;
        aj["id"] = a.id;
        aj["name"] = a.name;
        aj["type"] = a.type;
        assetsJson.append(aj);
    }
    respJson["assets"] = assetsJson;

    auto alarms = rules::AlarmEngine::getInstance().getActiveAlarms();
    Json::Value alarmsJson(Json::arrayValue);
    for (const auto& al : alarms) {
        Json::Value alj;
        alj["id"] = al.id;
        alj["asset_id"] = al.asset_id;
        alj["message"] = al.message;
        alj["state"] = "ACTIVE";
        alarmsJson.append(alj);
    }
    respJson["alarms"] = alarmsJson;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(respJson);
    callback(resp);
}

void RestCtrl::resolveAIIntent(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto jsonObj = req->getJsonObject();
    if (!jsonObj) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string intent = jsonObj->isMember("intent") ? (*jsonObj)["intent"].asString() : "";
    std::string target = jsonObj->isMember("target") ? (*jsonObj)["target"].asString() : "";
    
    Json::Value responseJson;
    responseJson["success"] = false;

    // 1. Validate Target Deterministically
    auto assets = storage::SQLiteRepository::getInstance().getAllAssets();
    bool targetFound = false;
    for (const auto& a : assets) {
        if (a.id == target || a.name == target) {
            targetFound = true;
            target = a.id; // Normalize to canonical ID
            break;
        }
    }

    if (!targetFound && !target.empty()) {
        responseJson["error"] = "C++ Validation failed: Target asset '" + target + "' does not exist or has been renamed/removed in the current context revision.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(responseJson);
        callback(resp);
        return;
    }

    // 2. Generate HMI JSON based on Intent
    Json::Value hmi(Json::objectValue);
    hmi["type"] = "canvas_view";
    hmi["target"] = target;
    hmi["intent"] = intent;
    
    Json::Value components(Json::arrayValue);
    if (jsonObj->isMember("properties") && (*jsonObj)["properties"].isArray()) {
        for (const auto& prop : (*jsonObj)["properties"]) {
            std::string p = prop.asString();
            Json::Value comp(Json::objectValue);
            comp["type"] = "metric_card";
            comp["binding"] = target + "." + p;
            comp["label"] = p;
            components.append(comp);
        }
    }
    
    // Inject Live Alarms directly into the canvas
    auto alarms = rules::AlarmEngine::getInstance().getActiveAlarms();
    for (const auto& al : alarms) {
        if (target.empty() || al.asset_id == target) {
            Json::Value comp(Json::objectValue);
            comp["type"] = "alarm_banner";
            comp["message"] = al.message;
            comp["priority"] = al.priority == domain::AlarmPriority::CRITICAL ? "CRITICAL" : "HIGH";
            components.append(comp);
        }
    }

    if (components.empty() && intent != "show_overview") {
         responseJson["error"] = "C++ Validation failed: Could not resolve valid bindings for the requested intent.";
         auto resp = drogon::HttpResponse::newHttpJsonResponse(responseJson);
         callback(resp);
         return;
    }

    hmi["components"] = components;
    responseJson["success"] = true;
    responseJson["hmi_json"] = hmi;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(responseJson);
    callback(resp);
}

} // namespace api
} // namespace ctm
