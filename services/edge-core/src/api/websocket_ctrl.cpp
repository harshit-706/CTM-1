#include "websocket_ctrl.h"
#include <iostream>

namespace ctm {
namespace api {

std::mutex WebSocketCtrl::connections_mutex_;
std::unordered_set<drogon::WebSocketConnectionPtr> WebSocketCtrl::connections_;

void WebSocketCtrl::handleNewMessage(const drogon::WebSocketConnectionPtr& wsConnPtr,
                                     std::string&& message,
                                     const drogon::WebSocketMessageType& type) {
    // We primarily push data to the client, but handle ping/pong if needed
    if (type == drogon::WebSocketMessageType::Ping) {
        wsConnPtr->send("pong");
    }
}

void WebSocketCtrl::handleNewConnection(const drogon::HttpRequestPtr& req,
                                        const drogon::WebSocketConnectionPtr& wsConnPtr) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.insert(wsConnPtr);
    std::cout << "New WebSocket connection established." << std::endl;
}

void WebSocketCtrl::handleConnectionClosed(const drogon::WebSocketConnectionPtr& wsConnPtr) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(wsConnPtr);
    std::cout << "WebSocket connection closed." << std::endl;
}

void WebSocketCtrl::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto const& conn : connections_) {
        conn->send(message);
    }
}

} // namespace api
} // namespace ctm
