#pragma once
#include "../domain/types.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace ctm {
namespace rules {

class AlarmEngine {
public:
    static AlarmEngine& getInstance() {
        static AlarmEngine instance;
        return instance;
    }

    void evaluateTelemetry(const domain::TelemetryEvent& event);
    std::vector<domain::Alarm> getActiveAlarms() const;
    std::optional<domain::Alarm> getAlarm(const std::string& id) const;
    
    // State transitions
    bool acknowledgeAlarm(const std::string& id, const std::string& user);
    bool clearAlarm(const std::string& id, const std::string& user);

private:
    AlarmEngine() = default;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, domain::Alarm> active_alarms_;
    
    void checkThresholds(const domain::TelemetryEvent& event);
    void createOrUpdateAlarm(const std::string& asset_id, const std::string& condition, const std::string& message, domain::AlarmPriority priority, uint64_t timestamp);
};

} // namespace rules
} // namespace ctm
