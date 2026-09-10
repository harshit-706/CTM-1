#pragma once
#include <drogon/WebSocketController.h>
#include <mutex>
#include <unordered_set>

namespace ctm {
namespace api {

class WebSocketCtrl : public drogon::WebSocketController<WebSocketCtrl> {
public:
    void handleNewMessage(const drogon::WebSocketConnectionPtr& wsConnPtr,
                          std::string&& message,
                          const drogon::WebSocketMessageType& type) override;
    
    void handleNewConnection(const drogon::HttpRequestPtr& req,
                             const drogon::WebSocketConnectionPtr& wsConnPtr) override;
    
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& wsConnPtr) override;

    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/live", drogon::Get);
    WS_PATH_LIST_END

    // Helper to broadcast to all connected clients
    static void broadcast(const std::string& message);

private:
    static std::mutex connections_mutex_;
    static std::unordered_set<drogon::WebSocketConnectionPtr> connections_;
};

} // namespace api
} // namespace ctm
