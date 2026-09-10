#include "alarm_engine.h"
#include "../api/websocket_ctrl.h"
#include <iostream>
#include <json/json.h>

namespace ctm {
namespace rules {

void AlarmEngine::evaluateTelemetry(const domain::TelemetryEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    checkThresholds(event);
}

void AlarmEngine::checkThresholds(const domain::TelemetryEvent& event) {
    if (event.tag_id == "vibration" && event.value > 2.0) {
        createOrUpdateAlarm(event.asset_id, "high_vibration", "Vibration Deviation Detected: > 2.0 mm/s", domain::AlarmPriority::HIGH, event.timestamp_ns);
    }
    if (event.tag_id == "current_a" && event.value > 20.0) {
        createOrUpdateAlarm(event.asset_id, "overcurrent", "VFD Overcurrent Trip", domain::AlarmPriority::CRITICAL, event.timestamp_ns);
    }
    if (event.tag_id == "fault_active") {
        if (event.value > 0.0) {
            createOrUpdateAlarm(event.asset_id, "sim_fault", "Simulated Fault Active", domain::AlarmPriority::CRITICAL, event.timestamp_ns);
        } else {
            clearAlarm(event.asset_id + "_sim_fault", "System");
        }
    }
}

void AlarmEngine::createOrUpdateAlarm(const std::string& asset_id, const std::string& condition, const std::string& message, domain::AlarmPriority priority, uint64_t timestamp) {
    std::string alarm_id = asset_id + "_" + condition;
    
    if (active_alarms_.find(alarm_id) == active_alarms_.end()) {
        domain::Alarm alarm;
        alarm.id = alarm_id;
        alarm.asset_id = asset_id;
        alarm.priority = priority;
        alarm.message = message;
        alarm.state = domain::AlarmState::ACTIVE;
        alarm.timestamp = timestamp;
        
        active_alarms_[alarm_id] = alarm;
        
        Json::Value wsMsg;
        wsMsg["type"] = "alarm.created";
        wsMsg["timestamp"] = alarm.timestamp;
        wsMsg["payload"]["id"] = alarm.id;
        wsMsg["payload"]["asset_id"] = alarm.asset_id;
        wsMsg["payload"]["message"] = alarm.message;
        wsMsg["payload"]["state"] = "ACTIVE";
        
        Json::FastWriter writer;
        api::WebSocketCtrl::broadcast(writer.write(wsMsg));
    }
}

bool AlarmEngine::acknowledgeAlarm(const std::string& id, const std::string& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_alarms_.find(id);
    if (it != active_alarms_.end() && it->second.state == domain::AlarmState::ACTIVE) {
        it->second.state = domain::AlarmState::ACKNOWLEDGED;
        it->second.ack_user = user;
        
        Json::Value wsMsg;
        wsMsg["type"] = "alarm.updated";
        wsMsg["payload"]["id"] = id;
        wsMsg["payload"]["state"] = "ACKNOWLEDGED";
        wsMsg["payload"]["ack_user"] = user;
        Json::FastWriter writer;
        api::WebSocketCtrl::broadcast(writer.write(wsMsg));
        return true;
    }
    return false;
}

bool AlarmEngine::clearAlarm(const std::string& id, const std::string& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_alarms_.find(id);
    if (it != active_alarms_.end()) {
        it->second.state = domain::AlarmState::CLEARED;
        
        Json::Value wsMsg;
        wsMsg["type"] = "alarm.cleared";
        wsMsg["payload"]["id"] = id;
        wsMsg["payload"]["clear_user"] = user;
        Json::FastWriter writer;
        api::WebSocketCtrl::broadcast(writer.write(wsMsg));
        
        active_alarms_.erase(it);
        return true;
    }
    return false;
}

std::vector<domain::Alarm> AlarmEngine::getActiveAlarms() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<domain::Alarm> results;
    for (const auto& pair : active_alarms_) {
        results.push_back(pair.second);
    }
    return results;
}

std::optional<domain::Alarm> AlarmEngine::getAlarm(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_alarms_.find(id);
    if (it != active_alarms_.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace rules
} // namespace ctm
