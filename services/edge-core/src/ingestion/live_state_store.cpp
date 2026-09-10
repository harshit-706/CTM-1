#include "live_state_store.h"

namespace ctm {
namespace ingestion {

void LiveStateStore::updateTelemetry(const domain::TelemetryEvent& event) {
    std::unique_lock lock(mutex_);
    store_[event.asset_id][event.tag_id] = event;
}

std::optional<domain::TelemetryEvent> LiveStateStore::getLatestTelemetry(const std::string& asset_id, const std::string& tag_id) const {
    std::shared_lock lock(mutex_);
    auto asset_it = store_.find(asset_id);
    if (asset_it != store_.end()) {
        auto tag_it = asset_it->second.find(tag_id);
        if (tag_it != asset_it->second.end()) {
            return tag_it->second;
        }
    }
    return std::nullopt;
}

std::vector<domain::TelemetryEvent> LiveStateStore::getAllLatestTelemetry() const {
    std::shared_lock lock(mutex_);
    std::vector<domain::TelemetryEvent> results;
    for (const auto& [asset_id, tags] : store_) {
        for (const auto& [tag_id, event] : tags) {
            results.push_back(event);
        }
    }
    return results;
}

} // namespace ingestion
} // namespace ctm
