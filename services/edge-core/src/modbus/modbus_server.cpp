// ============================================================================
// src/modbus/modbus_server.cpp
// CTM-1 — Modbus TCP Slave Server Implementation
//
// Only compiled when CTM_MODBUS_ENABLED=1 (libmodbus found at configure time).
// ============================================================================

#include "modbus_server.h"

#if CTM_MODBUS_ENABLED

#include <modbus/modbus.h>   // libmodbus C API
#include <modbus/modbus-tcp.h>

#include <cerrno>
#include <cstring>
#include <cmath>
#include <iostream>
#include <cassert>

// POSIX socket headers (available on Linux/macOS)
#if defined(__linux__) || defined(__APPLE__)
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#endif

namespace ctm {
namespace modbus {

// ── Public: start ─────────────────────────────────────────────────────────
bool ModbusServer::start(int port) {
    if (running_.load(std::memory_order_acquire)) {
        std::cerr << "[ModbusServer] Already running on port " << port << "\n";
        return false;
    }

    // Create libmodbus TCP context
    ctx_ = modbus_new_tcp("0.0.0.0", port);
    if (!ctx_) {
        std::cerr << "[ModbusServer] modbus_new_tcp failed: " << modbus_strerror(errno) << "\n";
        return false;
    }

#ifndef NDEBUG
    modbus_set_debug(ctx_, FALSE);   // Set TRUE to dump raw PDUs in debug builds
#endif

    // Response timeout: 500 ms
    modbus_set_response_timeout(ctx_, 0, 500'000);

    running_.store(true, std::memory_order_release);

    // Spawn daemon thread — server_thread_ keeps running until stop() is called
    server_thread_ = std::thread([this, port]() {
        serverLoop(port);
    });

    std::cout << "[ModbusServer] Modbus TCP slave listening on port " << port
              << " (unit ID 1)\n"
              << "               Register map: "
              << MODBUS_MAX_ASSETS << " assets × "
              << MODBUS_REGS_PER_ASSET << " regs = "
              << MODBUS_TOTAL_REGISTERS << " total holding registers\n";
    return true;
}

// ── Public: stop ──────────────────────────────────────────────────────────
void ModbusServer::stop() {
    running_.store(false, std::memory_order_release);

    if (ctx_) {
        modbus_close(ctx_);
        modbus_free(ctx_);
        ctx_ = nullptr;
    }

    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    std::cout << "[ModbusServer] Stopped.\n";
}

// ── Private: serverLoop ───────────────────────────────────────────────────
void ModbusServer::serverLoop(int port) {
    // Allocate libmodbus register mapping from our register array
    modbus_mapping_t* mb_mapping = modbus_mapping_new(
        0,                          // nb_bits (coils)
        0,                          // nb_input_bits (discrete inputs)
        MODBUS_TOTAL_REGISTERS,     // nb_registers (holding registers — read/write)
        MODBUS_TOTAL_REGISTERS      // nb_input_registers (read-only mirror)
    );

    if (!mb_mapping) {
        std::cerr << "[ModbusServer] modbus_mapping_new failed: "
                  << modbus_strerror(errno) << "\n";
        running_.store(false, std::memory_order_release);
        return;
    }

    // Sync initial register state to libmodbus mapping
    {
        std::shared_lock lock(registers_mutex_);
        std::memcpy(mb_mapping->tab_registers,
                    registers_,
                    sizeof(uint16_t) * MODBUS_TOTAL_REGISTERS);
    }

    // TCP listen socket
    int server_socket = modbus_tcp_listen(ctx_, 8); // backlog=8
    if (server_socket == -1) {
        std::cerr << "[ModbusServer] modbus_tcp_listen failed: "
                  << modbus_strerror(errno) << "\n";
        modbus_mapping_free(mb_mapping);
        running_.store(false, std::memory_order_release);
        return;
    }

    std::cout << "[ModbusServer] Accepting Modbus TCP connections on port "
              << port << "\n";

    // ── Main accept loop ───────────────────────────────────────────────
    while (running_.load(std::memory_order_acquire)) {
        // modbus_tcp_accept blocks until a client connects (or error)
        int client_socket = modbus_tcp_accept(ctx_, &server_socket);
        if (client_socket == -1) {
            if (running_.load(std::memory_order_acquire)) {
                std::cerr << "[ModbusServer] Accept error: "
                          << modbus_strerror(errno) << "\n";
            }
            break;
        }

        // ── Per-client request loop ────────────────────────────────────
        // (Simple single-client model — one client at a time)
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc;

        while (running_.load(std::memory_order_acquire)) {
            rc = modbus_receive(ctx_, query);
            if (rc <= 0) {
                // Client disconnected or error
                break;
            }

            // Sync our register array into libmodbus mapping before replying
            {
                std::shared_lock lock(registers_mutex_);
                std::memcpy(mb_mapping->tab_registers,
                            registers_,
                            sizeof(uint16_t) * MODBUS_TOTAL_REGISTERS);
                std::memcpy(mb_mapping->tab_input_registers,
                            registers_,
                            sizeof(uint16_t) * MODBUS_TOTAL_REGISTERS);
            }

            modbus_reply(ctx_, query, rc, mb_mapping);
        }

        // Clean up client socket
#if defined(__linux__) || defined(__APPLE__)
        ::close(client_socket);
#endif
        std::cout << "[ModbusServer] Client disconnected.\n";
    }

    modbus_mapping_free(mb_mapping);
#if defined(__linux__) || defined(__APPLE__)
    ::close(server_socket);
#endif
}

// ── Public: writeTagValue ─────────────────────────────────────────────────
void ModbusServer::writeTagValue(const std::string& asset_id,
                                  const std::string& tag_name,
                                  double             value) {
    // ── Determine or assign register offset ───────────────────────────
    const std::string key = asset_id + ":" + tag_name;

    int slot    = -1;
    int offset  = -1;

    {
        // Try read-lock first (hot path — asset already known)
        std::shared_lock rlock(slot_map_mutex_);
        auto sit = slot_map_.find(asset_id);
        if (sit != slot_map_.end()) {
            slot = sit->second;
            auto tit = tag_offsets_.find(key);
            if (tit != tag_offsets_.end()) {
                offset = tit->second;
            }
        }
    }

    if (slot == -1) {
        // Write-lock: assign new slot
        std::unique_lock wlock(slot_map_mutex_);
        slot = assignSlot(asset_id);
    }

    if (offset == -1) {
        // Write-lock: assign tag offset within slot
        std::unique_lock wlock(slot_map_mutex_);
        // Re-check after acquiring write lock
        auto tit = tag_offsets_.find(key);
        if (tit != tag_offsets_.end()) {
            offset = tit->second;
        } else {
            // Count how many tags this asset already has
            int tag_count = 0;
            for (const auto& [k, v] : tag_offsets_) {
                if (k.rfind(asset_id + ":", 0) == 0) {
                    ++tag_count;
                }
            }
            // Each float32 = 2 registers; status at +0, tags start at +1
            // Max 4 float32 tags per slot (+1,+2), (+3,+4), (+5,+6), (+7,+8)
            if (tag_count < 4) {
                offset = 1 + tag_count * 2;  // MSW offset within slot
                tag_offsets_[key] = offset;
            } else {
                // Slot full — silently discard (log in debug)
                return;
            }
        }
    }

    // ── Write float32 into two holding registers ──────────────────────
    const int base_reg = slot * MODBUS_REGS_PER_ASSET + offset;
    if (base_reg + 1 >= MODBUS_TOTAL_REGISTERS) return;

    uint16_t msw = 0, lsw = 0;
    float32ToRegisters(value, msw, lsw);

    std::unique_lock wlock(registers_mutex_);
    registers_[base_reg]     = msw;
    registers_[base_reg + 1] = lsw;
}

// ── Public: writeAssetStatus ──────────────────────────────────────────────
void ModbusServer::writeAssetStatus(const std::string& asset_id, bool online) {
    int slot = getSlot(asset_id);
    if (slot < 0) {
        // Auto-assign slot on first status write
        std::unique_lock wlock(slot_map_mutex_);
        slot = assignSlot(asset_id);
    }

    const int status_reg = slot * MODBUS_REGS_PER_ASSET;  // Register +0 = status
    std::unique_lock wlock(registers_mutex_);
    registers_[status_reg] = online ? 1U : 0U;
}

// ── Public: dumpRegisters ─────────────────────────────────────────────────
std::vector<uint16_t> ModbusServer::dumpRegisters() const {
    std::shared_lock lock(registers_mutex_);
    return std::vector<uint16_t>(registers_,
                                  registers_ + MODBUS_TOTAL_REGISTERS);
}

// ── Public: getSlot ───────────────────────────────────────────────────────
int ModbusServer::getSlot(const std::string& asset_id) const {
    std::shared_lock lock(slot_map_mutex_);
    auto it = slot_map_.find(asset_id);
    return (it != slot_map_.end()) ? it->second : -1;
}

// ── Private: assignSlot ───────────────────────────────────────────────────
// MUST be called with slot_map_mutex_ held (write lock)
int ModbusServer::assignSlot(const std::string& asset_id) {
    // Check if already assigned (TOCTOU guard)
    auto it = slot_map_.find(asset_id);
    if (it != slot_map_.end()) return it->second;

    int next_slot = static_cast<int>(slot_map_.size());
    if (next_slot >= MODBUS_MAX_ASSETS) {
        std::cerr << "[ModbusServer] WARNING: exceeded MODBUS_MAX_ASSETS ("
                  << MODBUS_MAX_ASSETS << "). Asset " << asset_id
                  << " will NOT be mapped to Modbus registers.\n";
        return -1;
    }
    slot_map_[asset_id] = next_slot;
    std::cout << "[ModbusServer] Mapped asset '" << asset_id
              << "' → slot " << next_slot
              << " (regs " << (next_slot * MODBUS_REGS_PER_ASSET)
              << "-" << (next_slot * MODBUS_REGS_PER_ASSET + MODBUS_REGS_PER_ASSET - 1)
              << ")\n";
    return next_slot;
}

// ── Private: float32ToRegisters ───────────────────────────────────────────
// Encodes a double as float32 → two big-endian 16-bit words.
// Big-endian word order: [MSW][LSW] matches Modbus convention.
void ModbusServer::float32ToRegisters(double value,
                                       uint16_t& msw,
                                       uint16_t& lsw) noexcept {
    // Clamp to float range
    if (std::isnan(value) || std::isinf(value)) {
        msw = lsw = 0;
        return;
    }
    float fval = static_cast<float>(value);
    uint32_t bits = 0;
    static_assert(sizeof(float) == sizeof(uint32_t), "float must be 32-bit");
    std::memcpy(&bits, &fval, sizeof(float));
    msw = static_cast<uint16_t>((bits >> 16) & 0xFFFF);
    lsw = static_cast<uint16_t>(bits & 0xFFFF);
}

} // namespace modbus
} // namespace ctm

#endif // CTM_MODBUS_ENABLED
