#pragma once
#include <shared_mutex>
#include <unordered_map>
#include <string>
#include <vector>
#include "../domain/types.h"

namespace ctm {
namespace ingestion {

class LiveStateStore {
public:
    static LiveStateStore& getInstance() {
        static LiveStateStore instance;
        return instance;
    }

    void updateTelemetry(const domain::TelemetryEvent& event);
    std::optional<domain::TelemetryEvent> getLatestTelemetry(const std::string& asset_id, const std::string& tag_id) const;
    std::vector<domain::TelemetryEvent> getAllLatestTelemetry() const;

private:
    LiveStateStore() = default;
    
    mutable std::shared_mutex mutex_;
    // Map: asset_id -> (tag_id -> TelemetryEvent)
    std::unordered_map<std::string, std::unordered_map<std::string, domain::TelemetryEvent>> store_;
};

} // namespace ingestion
} // namespace ctm
