#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <optional>

namespace ctm {
namespace domain {

enum class Quality {
    GOOD,
    BAD,
    UNCERTAIN
};

enum class AssetStatus {
    ONLINE,
    OFFLINE,
    DEGRADED,
    FAULT
};

struct Tag {
    std::string id;
    std::string name;
    std::string unit;
    std::string type;
};

struct Sensor {
    std::string id;
    std::string type;
};

struct Asset {
    std::string id;
    std::string name;
    std::string type;
    std::string vendor;
    std::string model;
    std::string location;
    std::optional<std::string> parentId;
    int health; // 0-100
    AssetStatus status;
    std::string protocol;
    std::string endpoint;
    std::string firmware;
    std::string configurationVersion;
    std::vector<Tag> tags;
    std::vector<Sensor> sensors;
};

// Canonical Telemetry Event (from context)
struct TelemetryEvent {
    uint64_t timestamp_ns;
    std::string asset_id;
    std::string tag_id;
    double value;
    Quality quality;
    std::string source; // e.g. "SIMULATED"
};

enum class AlarmPriority {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

enum class AlarmState {
    ACTIVE,
    ACKNOWLEDGED,
    CLEARED
};

struct Alarm {
    std::string id;
    std::string asset_id;
    AlarmPriority priority;
    std::string message;
    AlarmState state;
    uint64_t timestamp;
    std::string ack_user;
    std::string clear_user;
};

} // namespace domain
} // namespace ctm
