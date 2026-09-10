#include "topology_ctrl.h"
#include <drogon/HttpClient.h>
#include <json/json.h>
#include <sstream>
#include <map>
#include <vector>
#include "../storage/sqlite_repository.h"
#include "../ingestion/live_state_store.h"
#include "websocket_ctrl.h"

namespace ctm {
namespace api {

void TopologyCtrl::publishTopology(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto jsonBody = req->getJsonObject();
    if (!jsonBody) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // 1. Server-side Validation
    std::map<std::string, std::vector<std::string>> allowedConnections = {
        {"plc", {"vfd", "soft_starter", "remote_io", "scada", "edge_gateway"}},
        {"pac", {"vfd", "soft_starter", "remote_io", "scada", "edge_gateway"}},
        {"remote_io", {"vfd", "soft_starter", "sensor"}},
        {"breaker", {"vfd", "soft_starter", "energy_meter", "motor"}},
        {"vfd", {"motor"}},
        {"soft_starter", {"motor"}},
        {"motor", {"pump", "compressor", "fan", "vibration_sensor", "temp_sensor"}},
        {"pump", {"pressure_sensor", "flow_sensor", "vibration_sensor"}},
        {"compressor", {"pressure_sensor", "temp_sensor", "vibration_sensor"}},
        {"fan", {"vibration_sensor"}},
        {"temp_sensor", {"plc", "pac", "remote_io", "edge_gateway"}},
        {"pressure_sensor", {"plc", "pac", "remote_io", "edge_gateway"}},
        {"flow_sensor", {"plc", "pac", "remote_io", "edge_gateway"}},
        {"vibration_sensor", {"plc", "pac", "remote_io", "edge_gateway"}},
        {"energy_meter", {"plc", "pac", "edge_gateway"}},
        {"edge_gateway", {"scada", "hmi"}}
    };

    const auto& nodes = (*jsonBody)["nodes"];
    const auto& edges = (*jsonBody)["edges"];

    std::map<std::string, std::string> nodeTypes;
    for (const auto& node : nodes) {
        nodeTypes[node["id"].asString()] = node["type"].asString();
    }

    for (const auto& edge : edges) {
        std::string src = edge["source"].asString();
        std::string tgt = edge["target"].asString();
        std::string srcType = nodeTypes[src];
        std::string tgtType = nodeTypes[tgt];

        auto it = allowedConnections.find(srcType);
        bool valid = false;
        if (it != allowedConnections.end()) {
            for (const auto& allowed : it->second) {
                if (allowed == tgtType) {
                    valid = true;
                    break;
                }
            }
        }
        
        if (!valid) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            resp->setBody("{\"error\": \"Validation failed: " + srcType + " cannot connect to " + tgtType + "\"}");
            callback(resp);
            return;
        }
    }

    // 2. Register to SQLite
    auto& db = ctm::storage::SQLiteRepository::getInstance();
    db.clearAssets(); // Remove old topology before inserting new
    
    for (const auto& node : nodes) {
        ctm::domain::Asset a;
        a.id = node["id"].asString();
        a.name = node["label"].asString();
        a.type = node["type"].asString();
        a.vendor = "";
        a.model = "";
        a.location = "Topology Canvas";
        a.firmware = "";
        a.configurationVersion = "1.0";
        db.saveAsset(a);
    }
    
    db.logAuditEvent("TOPOLOGY_PUBLISH", "SYSTEM", "Published " + std::to_string(nodes.size()) + " assets");

    // 3. Forward to Python Simulator
    auto client = drogon::HttpClient::newHttpClient("http://127.0.0.1:8082");
    auto simReq = drogon::HttpRequest::newHttpRequest();
    simReq->setMethod(drogon::Post);
    simReq->setPath("/api/v1/topology");
    simReq->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    Json::FastWriter writer;
    simReq->setBody(writer.write(*jsonBody));
    
    client->sendRequest(simReq, [callback](drogon::ReqResult result, const drogon::HttpResponsePtr& simResp) {
        if (result == drogon::ReqResult::Ok && simResp) {
            // 4. WebSocket Broadcast
            ctm::api::WebSocketCtrl::broadcast("{\"type\": \"asset.registry.updated\"}");
            
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

void TopologyCtrl::listAssets(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto& db = ctm::storage::SQLiteRepository::getInstance();
    auto& store = ctm::ingestion::LiveStateStore::getInstance();
    
    auto allAssets = db.getAllAssets();
    auto allTelemetry = store.getAllLatestTelemetry();
    
    Json::Value assets(Json::arrayValue);
    for (auto& asset : allAssets) {
        bool hasTelemetry = false;
        for (const auto& tel : allTelemetry) {
            if (tel.asset_id == asset.id) {
                hasTelemetry = true;
                break;
            }
        }
        
        asset.status = hasTelemetry ? ctm::domain::AssetStatus::ONLINE : ctm::domain::AssetStatus::OFFLINE;
        
        Json::Value a;
        a["id"] = asset.id;
        a["name"] = asset.name;
        a["type"] = asset.type;
        a["vendor"] = asset.vendor;
        a["model"] = asset.model;
        a["location"] = asset.location;
        a["health"] = 100;
        
        std::string statusStr = "OFFLINE";
        switch (asset.status) {
            case ctm::domain::AssetStatus::ONLINE: statusStr = "ONLINE"; break;
            case ctm::domain::AssetStatus::DEGRADED: statusStr = "DEGRADED"; break;
            case ctm::domain::AssetStatus::FAULT: statusStr = "FAULT"; break;
            default: statusStr = "OFFLINE"; break;
        }
        a["status"] = statusStr;
        a["protocol"] = "SIMULATED";
        a["endpoint"] = "sim://" + asset.id;
        a["firmware"] = asset.firmware;
        a["configurationVersion"] = asset.configurationVersion;
        a["tags"] = Json::Value(Json::arrayValue);
        a["sensors"] = Json::Value(Json::arrayValue);
        assets.append(a);
    }
    
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    Json::Value out;
    out["assets"] = assets;
    Json::FastWriter writer;
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(writer.write(out));
    callback(resp);
}

} // namespace api
} // namespace ctm
