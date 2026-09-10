#pragma once
#include "../domain/types.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <deque>

namespace ctm {
namespace rules {

class CorrelationEngine {
public:
    static CorrelationEngine& getInstance() {
        static CorrelationEngine instance;
        return instance;
    }

    void evaluateTelemetry(const domain::TelemetryEvent& event);
    void checkIncidentConditions(const std::string& asset_id);

private:
    CorrelationEngine() = default;
    
    std::mutex mutex_;
    // Keep last N events per asset for pattern matching
    std::unordered_map<std::string, std::deque<domain::TelemetryEvent>> event_buffer_;
};

} // namespace rules
} // namespace ctm
