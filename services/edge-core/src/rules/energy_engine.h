#pragma once
#include "../domain/types.h"
#include <mutex>
#include <unordered_map>

namespace ctm {
namespace rules {

class EnergyEngine {
public:
    static EnergyEngine& getInstance() {
        static EnergyEngine instance;
        return instance;
    }

    void processTelemetry(const domain::TelemetryEvent& event);

private:
    EnergyEngine() = default;
    std::mutex mutex_;
    
    // Store latest power (kW) and flow (production proxy) per asset
    std::unordered_map<std::string, double> asset_power_kw_;
    std::unordered_map<std::string, double> asset_flow_;
    
    void calculateIntensity(const std::string& asset_id);
};

} // namespace rules
} // namespace ctm
