#include "energy_engine.h"
#include "../api/websocket_ctrl.h"
#include <json/json.h>
#include <iostream>

namespace ctm {
namespace rules {

void EnergyEngine::processTelemetry(const domain::TelemetryEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (event.tag_id == "power_kw") {
        asset_power_kw_[event.asset_id] = event.value;
        calculateIntensity(event.asset_id);
    } else if (event.tag_id == "flow" || event.tag_id == "speed_hz" || event.tag_id == "frequency_hz" || event.tag_id == "speed_rpm" || event.tag_id == "output_freq_hz") {
        asset_flow_[event.asset_id] = event.value;
        calculateIntensity(event.asset_id);
    }
}

void EnergyEngine::calculateIntensity(const std::string& asset_id) {
    if (asset_power_kw_.count(asset_id)) {
        double power = asset_power_kw_[asset_id];
        // Use flow/speed as production rate if available, else default to 1.0 just to show power
        double flow = asset_flow_.count(asset_id) && asset_flow_[asset_id] > 0 ? asset_flow_[asset_id] : 1.0;
        
        double intensity = power / flow; // simplified energy intensity
        double baseline = 0.40; // mock baseline
        double deviation = ((intensity - baseline) / baseline) * 100.0;
        
        Json::Value wsMsg;
        wsMsg["type"] = "energy.update";
        wsMsg["payload"]["asset_id"] = asset_id;
        wsMsg["payload"]["power_kw"] = power;
        wsMsg["payload"]["production_rate"] = flow;
        wsMsg["payload"]["intensity"] = intensity;
        wsMsg["payload"]["deviation_pct"] = deviation;
        
        Json::FastWriter writer;
        api::WebSocketCtrl::broadcast(writer.write(wsMsg));
    }
}

} // namespace rules
} // namespace ctm
