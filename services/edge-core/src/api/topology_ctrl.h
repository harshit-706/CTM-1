#pragma once
#include <drogon/HttpController.h>

namespace ctm {
namespace api {

class TopologyCtrl : public drogon::HttpController<TopologyCtrl, false> {
public:
    METHOD_LIST_BEGIN
    METHOD_ADD(TopologyCtrl::publishTopology, "/api/v1/topology", drogon::Post);
    METHOD_ADD(TopologyCtrl::listAssets, "/api/v1/assets", drogon::Get);
    METHOD_LIST_END

    void publishTopology(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Proxies the simulator's live asset registry (GET /api/v1/state on :8082)
    // and reshapes it into the canonical Asset[] contract from packages/schema/types.ts.
    // This is what the /assets page (and any future asset list) must call instead
    // of rendering hard-coded rows.
    void listAssets(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace api
} // namespace ctm
