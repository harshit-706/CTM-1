// ============================================================================
// src/main.cpp
// CTM-1 Edge Core — Entry Point
//
// Build: cmake --preset linux-debug-sanitize && cmake --build --preset linux-debug-sanitize
// Run:   ./build/linux-debug-sanitize/bin/edge-core
//
// NOTE: No .cpp files are #included here. Each translation unit is compiled
//       separately via CMake targets (cmake/Targets.cmake).
//       The previous #include "api/rest_ctrl.cpp" was an ODR violation.
// ============================================================================

// ── Standard library ───────────────────────────────────────────────────────
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

// ── Drogon HTTP/WebSocket framework ───────────────────────────────────────
#include <drogon/drogon.h>

// ── CTM-1 modules (headers only — implementations compiled separately) ────
#include "api/rest_ctrl.h"
#include "api/topology_ctrl.h"
#include "api/websocket_ctrl.h"
#include "ingestion/live_state_store.h"
#include "rules/alarm_engine.h"
#include "rules/correlation_engine.h"
#include "rules/energy_engine.h"
#include "storage/sqlite_repository.h"
#include "modbus/modbus_server.h"

// ── Configuration constants (overridable via env vars) ────────────────────
namespace cfg {
    static const char*  HTTP_HOST   = "0.0.0.0";
    static const int    HTTP_PORT   = 8080;
    static const char*  MODBUS_HOST = "0.0.0.0";
    static const int    MODBUS_PORT = 502;
    static const char*  DB_PATH     = "ctm1.db";
}

// ── Signal handling ───────────────────────────────────────────────────────
// Global atomic flag — set by SIGTERM/SIGINT, checked by Drogon's loop.
static std::atomic<bool> g_shutdown_requested{false};

static void handleSignal(int signum) noexcept {
    // async-signal-safe: only write to atomic and call async-safe functions
    g_shutdown_requested.store(true, std::memory_order_relaxed);
    // Gracefully terminate Drogon's event loop
    drogon::app().quit();
}

static void registerSignalHandlers() {
    struct sigaction sa{};
    sa.sa_handler = handleSignal;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    ::sigaction(SIGTERM, &sa, nullptr);   // Docker/systemd stop
    ::sigaction(SIGINT,  &sa, nullptr);   // Ctrl-C
    ::sigaction(SIGHUP,  &sa, nullptr);   // Terminal hangup / reload
    // Ignore SIGPIPE — raised by broken WebSocket/HTTP connections
    ::signal(SIGPIPE, SIG_IGN);
}

// ── Startup banner ────────────────────────────────────────────────────────
static void printBanner() {
    std::cout
        << "\n"
        << "  ┌────────────────────────────────────────────────────┐\n"
        << "  │  CTM-1 Edge Core v" CTM1_VERSION_MAJOR "." CTM1_VERSION_MINOR "." CTM1_VERSION_PATCH
        << "  [" CTM1_BUILD_TYPE "]" << "                    │\n"
        << "  │  Industrial Edge Gateway — HTTP / WebSocket / Modbus  │\n"
        << "  ├────────────────────────────────────────────────────┤\n"
        << "  │  HTTP  : http://"  << cfg::HTTP_HOST << ":" << cfg::HTTP_PORT  << "/api/v1       │\n"
        << "  │  WS    : ws://"    << cfg::HTTP_HOST << ":" << cfg::HTTP_PORT  << "/live          │\n"
#if CTM_MODBUS_ENABLED
        << "  │  Modbus: tcp://"   << cfg::MODBUS_HOST << ":" << cfg::MODBUS_PORT << " (holding regs)  │\n"
#else
        << "  │  Modbus: DISABLED (libmodbus not found at build)      │\n"
#endif
        << "  │  DB    : "         << cfg::DB_PATH << "                          │\n"
        << "  └────────────────────────────────────────────────────┘\n"
        << "\n";
}

// ── /api/v1/health endpoint ───────────────────────────────────────────────
static void registerHealthEndpoint() {
    drogon::app().registerHandler(
        "/api/v1/health",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        {
            Json::Value body;
            body["status"]           = "ok";
            body["service"]          = "ctm1-edge-core";
            body["version"]          = CTM1_VERSION_MAJOR "." CTM1_VERSION_MINOR "." CTM1_VERSION_PATCH;
            body["modbus_enabled"]   = (CTM_MODBUS_ENABLED != 0);
            body["modbus_running"]   = ctm::modbus::ModbusServer::getInstance().isRunning();

            auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
            resp->setStatusCode(drogon::k200OK);
            callback(resp);
        },
        {drogon::Get}
    );
}

// ── CORS middleware ───────────────────────────────────────────────────────
static void registerCors() {
    drogon::app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr&, const drogon::HttpResponsePtr& resp) {
            resp->addHeader("Access-Control-Allow-Origin",  "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        }
    );

    // Catch-all OPTIONS preflight
    auto preflight = [](const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&& cb)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        cb(resp);
    };

    for (const char* path : {
            "/api/v1/topology",
            "/api/v1/assets",
            "/api/v1/telemetry",
            "/api/v1/simulator/control",
            "/api/v1/health",
            "/api/v1/alarms",
            "/api/v1/alarms/{id}/ack",
            "/api/v1/alarms/{id}/clear"
        }) {
        drogon::app().registerHandler(path, preflight, {drogon::Options});
    }
}

// ── Controller registration ───────────────────────────────────────────────
static void registerControllers() {
    // ── REST — telemetry ingest & simulator control ────────────────────
    auto restCtrl = std::make_shared<ctm::api::RestCtrl>();

    drogon::app().registerHandler(
        "/api/v1/telemetry",
        [restCtrl](const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            restCtrl->ingestTelemetry(req, std::move(cb));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/v1/simulator/control",
        [restCtrl](const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            restCtrl->simulatorControl(req, std::move(cb));
        },
        {drogon::Post}
    );

    // ── Topology — publish & list assets ──────────────────────────────
    auto topoCtrl = std::make_shared<ctm::api::TopologyCtrl>();

    drogon::app().registerHandler(
        "/api/v1/topology",
        [topoCtrl](const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            topoCtrl->publishTopology(req, std::move(cb));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/v1/assets",
        [topoCtrl](const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            topoCtrl->listAssets(req, std::move(cb));
        },
        {drogon::Get}
    );
}

// ── main ──────────────────────────────────────────────────────────────────
int main(int /*argc*/, char* /*argv*/[]) {

    // 1. OS signals — must be registered before any threads are spawned
    registerSignalHandlers();

    // 2. SQLite persistent storage
    if (!ctm::storage::SQLiteRepository::getInstance().initialize(cfg::DB_PATH)) {
        std::cerr << "[main] FATAL: SQLite initialization failed: " << cfg::DB_PATH << "\n";
        return EXIT_FAILURE;
    }
    std::cout << "[main] SQLite Registry initialized: " << cfg::DB_PATH << "\n";

    // 3. Modbus TCP slave (non-blocking daemon thread)
#if CTM_MODBUS_ENABLED
    if (!ctm::modbus::ModbusServer::getInstance().start(cfg::MODBUS_PORT)) {
        // Non-fatal: edge core works without Modbus
        std::cerr << "[main] WARNING: Modbus TCP server failed to start on port "
                  << cfg::MODBUS_PORT << " — Modbus disabled for this session.\n";
    }
#endif

    // 4. Drogon HTTP + WebSocket server
    drogon::app().addListener(cfg::HTTP_HOST, cfg::HTTP_PORT);
    drogon::app().setThreadNum(4);   // Drogon I/O thread pool size
    drogon::app().disableSigsegfHandler();  // We handle our own signals

    // 5. Register middleware & routes
    registerCors();
    registerHealthEndpoint();
    registerControllers();

    // 6. Print startup banner
    printBanner();

    // 7. Enter Drogon event loop (blocks until drogon::app().quit() called)
    drogon::app().run();

    // 8. Graceful shutdown sequence
    std::cout << "\n[main] Shutdown signal received — cleaning up...\n";

#if CTM_MODBUS_ENABLED
    ctm::modbus::ModbusServer::getInstance().stop();
#endif

    std::cout << "[main] CTM-1 Edge Core stopped. Goodbye.\n";
    return EXIT_SUCCESS;
}
