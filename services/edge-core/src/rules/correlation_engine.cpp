#include "correlation_engine.h"
#include "../api/websocket_ctrl.h"
#include <iostream>
#include <json/json.h>

namespace ctm {
namespace rules {

void CorrelationEngine::evaluateTelemetry(const domain::TelemetryEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& buffer = event_buffer_[event.asset_id];
    buffer.push_back(event);
    if (buffer.size() > 50) {
        buffer.pop_front();
    }
    
    checkIncidentConditions(event.asset_id);
}

void CorrelationEngine::checkIncidentConditions(const std::string& asset_id) {
    auto& buffer = event_buffer_[asset_id];
    if (buffer.empty()) return;

    auto latest = buffer.back();
    
    bool overcurrent = (latest.tag_id == "current_a" && latest.value > 20.0);
    bool fault_active = (latest.tag_id == "fault_active" && latest.value > 0.0);
    
    // Check if we already triggered an incident for this asset
    static std::unordered_map<std::string, bool> incident_active;
    
    if (overcurrent || fault_active) {
        if (!incident_active[asset_id]) {
            incident_active[asset_id] = true;
            std::cout << "[CORRELATION] Incident hypothesis generated for " << asset_id << std::endl;
            
            Json::Value wsMsg;
            wsMsg["type"] = "incident.created";
            wsMsg["timestamp"] = latest.timestamp_ns;
            wsMsg["payload"]["id"] = asset_id + "_incident";
            wsMsg["payload"]["asset_id"] = asset_id;
            wsMsg["payload"]["title"] = "Process Anomaly on " + asset_id;
            wsMsg["payload"]["hypothesis"] = fault_active ? "Simulated Fault Detected" : "Degraded component leading to overcurrent trip";
            
            Json::Value event1;
            event1["timestamp"] = latest.timestamp_ns - 10000000;
            event1["description"] = "Process drift detected";
            event1["type"] = "STATE_CHANGE";
            
            Json::Value event2;
            event2["timestamp"] = latest.timestamp_ns;
            event2["description"] = fault_active ? "Simulated Fault" : "VFD Overcurrent Trip";
            event2["type"] = "ALARM";
            
            wsMsg["payload"]["timeline"].append(event1);
            wsMsg["payload"]["timeline"].append(event2);
            
            Json::FastWriter writer;
            api::WebSocketCtrl::broadcast(writer.write(wsMsg));
        }
    } else if (latest.tag_id == "fault_active" && latest.value == 0.0) {
        // Reset the incident state when the fault is cleared
        incident_active[asset_id] = false;
    }
}

} // namespace rules
} // namespace ctm
