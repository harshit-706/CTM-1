#pragma once
// ============================================================================
// src/modbus/modbus_server.h
// CTM-1 — Modbus TCP Slave Server
//
// Maps live asset telemetry tags to Modbus holding registers (FC03/FC06/FC16).
// Runs as a background daemon thread. Thread-safe via shared_mutex.
//
// Register Map (16-bit registers, big-endian):
//   Each asset slot = 10 registers (20 bytes)
//   Slot 0: registers   0-9   → asset[0] tags
//   Slot 1: registers  10-19  → asset[1] tags
//   ...
//   Slot N: registers N*10 .. N*10+9
//
//   Within each slot:
//     +0  : status      (1=ONLINE, 0=OFFLINE)
//     +1  : tag[0] MSW  (float32 split into two 16-bit words, big-endian)
//     +2  : tag[0] LSW
//     +3  : tag[1] MSW
//     +4  : tag[1] LSW
//     +5  : tag[2] MSW
//     +6  : tag[2] LSW
//     +7  : tag[3] MSW
//     +8  : tag[3] LSW
//     +9  : reserved
//
// Port: 502 (standard Modbus TCP)
// Unit ID: 1
// ============================================================================

#if CTM_MODBUS_ENABLED

#include <atomic>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>

// Forward-declare libmodbus C struct to avoid exposing C header globally
struct _modbus;
typedef struct _modbus modbus_t;

namespace ctm {
namespace modbus {

// Maximum assets mapped to Modbus registers
static constexpr int MODBUS_MAX_ASSETS      = 64;
static constexpr int MODBUS_REGS_PER_ASSET  = 10;
static constexpr int MODBUS_TOTAL_REGISTERS = MODBUS_MAX_ASSETS * MODBUS_REGS_PER_ASSET;
static constexpr int MODBUS_TCP_PORT        = 502;

// ── ModbusServer ────────────────────────────────────────────────────────
class ModbusServer {
public:
    // Singleton — consistent with existing CTM-1 engine pattern
    static ModbusServer& getInstance() {
        static ModbusServer instance;
        return instance;
    }

    ModbusServer(const ModbusServer&)            = delete;
    ModbusServer& operator=(const ModbusServer&) = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────
    /// Start the Modbus TCP server on the given port (default 502).
    /// Returns false if already running or libmodbus init fails.
    bool start(int port = MODBUS_TCP_PORT);

    /// Graceful shutdown — waits for background thread to exit.
    void stop();

    bool isRunning() const noexcept { return running_.load(std::memory_order_acquire); }

    // ── Register write API (called by ingestion layer) ─────────────────
    /// Write a float32 value for (asset_id, tag_name) into the register map.
    /// Thread-safe. Unknown assets are auto-assigned to the next free slot.
    void writeTagValue(const std::string& asset_id,
                       const std::string& tag_name,
                       double             value);

    /// Set asset online/offline status register.
    void writeAssetStatus(const std::string& asset_id, bool online);

    // ── Register read API (for diagnostics) ───────────────────────────
    /// Copy current register map snapshot. Returns vector of MODBUS_TOTAL_REGISTERS words.
    std::vector<uint16_t> dumpRegisters() const;

    /// Resolve asset_id → slot index (-1 if not mapped)
    int getSlot(const std::string& asset_id) const;

private:
    ModbusServer() = default;
    ~ModbusServer() { stop(); }

    // ── Background thread entry point ──────────────────────────────────
    void serverLoop(int port);

    // ── Register helpers ───────────────────────────────────────────────
    /// Encode float32 as two big-endian 16-bit words (float AB CD → [AB CD])
    static void float32ToRegisters(double value, uint16_t& msw, uint16_t& lsw) noexcept;

    /// Assign asset to a slot (call with write lock held)
    int assignSlot(const std::string& asset_id);

    // ── State ──────────────────────────────────────────────────────────
    mutable std::shared_mutex   registers_mutex_;
    uint16_t                    registers_[MODBUS_TOTAL_REGISTERS] = {};

    mutable std::shared_mutex   slot_map_mutex_;
    std::unordered_map<std::string, int>   slot_map_;   // asset_id → slot index
    std::unordered_map<std::string, int>   tag_offsets_; // "asset:tag" → register offset within slot

    std::atomic<bool>   running_{false};
    std::thread         server_thread_;
    modbus_t*           ctx_{nullptr};     // libmodbus context (builder stage only)
};

} // namespace modbus
} // namespace ctm

#else // CTM_MODBUS_ENABLED == 0

// Stub header — Modbus not available, provide no-op API
namespace ctm {
namespace modbus {
struct ModbusServer {
    static ModbusServer& getInstance() {
        static ModbusServer s;
        return s;
    }
    bool start(int = 502)                                                    { return false; }
    void stop()                                                              {}
    bool isRunning() const noexcept                                          { return false; }
    void writeTagValue(const std::string&, const std::string&, double)      {}
    void writeAssetStatus(const std::string&, bool)                         {}
};
} // namespace modbus
} // namespace ctm

#endif // CTM_MODBUS_ENABLED
