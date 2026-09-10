#include <drogon/drogon.h>
#include <iostream>
#include "api/rest_ctrl.cpp"
#include "api/topology_ctrl.cpp"
#include "storage/sqlite_repository.h"

int main() {
    std::cout << "CTM-1 Edge Core starting..." << std::endl;
    
    // Initialize SQLite Database
    ctm::storage::SQLiteRepository::getInstance().initialize("ctm1.db");
    
    // Initialize Drogon framework
    drogon::app().addListener("0.0.0.0", 8080);
    
    std::cout << "Listening on http://0.0.0.0:8080" << std::endl;
    std::cout << "WebSocket endpoint at ws://0.0.0.0:8080/live" << std::endl;
    
    // ── Global CORS ──
    // Attach Access-Control headers to every HTTP response so the
    // SvelteKit dev server (localhost:5173) can reach this API.
    drogon::app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr&, const drogon::HttpResponsePtr& resp) {
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        });

    // Catch-all OPTIONS handler for browser preflight
    auto optionsHandler = [](const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        callback(resp);
    };
    drogon::app().registerHandler("/api/v1/topology", optionsHandler, {drogon::Options});
    drogon::app().registerHandler("/api/v1/assets", optionsHandler, {drogon::Options});
    drogon::app().registerHandler("/api/v1/telemetry", optionsHandler, {drogon::Options});
    drogon::app().registerHandler("/api/v1/simulator/control", optionsHandler, {drogon::Options});
    
    // ── Controller registration ──
    // Explicitly register handlers to bypass MSVC linker/macro static initialization issues
    auto restCtrl = std::make_shared<ctm::api::RestCtrl>();
    drogon::app().registerHandler("/api/v1/telemetry", 
        [restCtrl](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            restCtrl->ingestTelemetry(req, std::move(callback));
        }, {drogon::Post});
        
    drogon::app().registerHandler("/api/v1/simulator/control", 
        [restCtrl](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            restCtrl->simulatorControl(req, std::move(callback));
        }, {drogon::Post});
        
    auto topoCtrl = std::make_shared<ctm::api::TopologyCtrl>();
    drogon::app().registerHandler("/api/v1/topology", 
        [topoCtrl](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            topoCtrl->publishTopology(req, std::move(callback));
        }, {drogon::Post});

    drogon::app().registerHandler("/api/v1/assets",
        [topoCtrl](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            topoCtrl->listAssets(req, std::move(callback));
        }, {drogon::Get});

    drogon::app().run();
    
    return 0;
}
