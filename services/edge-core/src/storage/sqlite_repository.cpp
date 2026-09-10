#include "sqlite_repository.h"
#include <iostream>
#include <sstream>

namespace ctm {
namespace storage {

SQLiteRepository& SQLiteRepository::getInstance() {
    static SQLiteRepository instance;
    return instance;
}

SQLiteRepository::~SQLiteRepository() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool SQLiteRepository::initialize(const std::string& dbPath) {
    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Failed to open SQLite database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    const char* createAssets = R"(
        CREATE TABLE IF NOT EXISTS assets (
            id TEXT PRIMARY KEY,
            name TEXT,
            type TEXT,
            vendor TEXT,
            model TEXT,
            location TEXT,
            firmware TEXT,
            config_version TEXT
        );
    )";
    
    const char* createAudit = R"(
        CREATE TABLE IF NOT EXISTS audit_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER,
            action TEXT,
            target TEXT,
            details TEXT
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db_, createAssets, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create assets table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    if (sqlite3_exec(db_, createAudit, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create audit table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    std::cout << "SQLite Registry initialized at " << dbPath << std::endl;
    return true;
}

bool SQLiteRepository::logAuditEvent(const std::string& action, const std::string& target, const std::string& details) {
    if (!db_) return false;
    
    const char* sql = "INSERT INTO audit_logs (timestamp, action, target, details) VALUES (strftime('%s','now'), ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, target.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, details.c_str(), -1, SQLITE_TRANSIENT);
    
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool SQLiteRepository::saveAsset(const domain::Asset& asset) {
    if (!db_) return false;
    
    const char* sql = "INSERT OR REPLACE INTO assets (id, name, type, vendor, model, location, firmware, config_version) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, asset.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, asset.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, asset.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, asset.vendor.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, asset.model.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, asset.location.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, asset.firmware.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, asset.configurationVersion.c_str(), -1, SQLITE_TRANSIENT);
    
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::optional<domain::Asset> SQLiteRepository::getAsset(const std::string& id) {
    if (!db_) return std::nullopt;
    
    const char* sql = "SELECT id, name, type, vendor, model, location, firmware, config_version FROM assets WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::Asset asset;
        asset.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        asset.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        asset.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        asset.vendor = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        asset.model = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        asset.location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        asset.firmware = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        asset.configurationVersion = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        asset.status = ctm::domain::AssetStatus::OFFLINE;
        sqlite3_finalize(stmt);
        return asset;
    }
    
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<domain::Asset> SQLiteRepository::getAllAssets() {
    std::vector<domain::Asset> assets;
    if (!db_) return assets;
    
    const char* sql = "SELECT id, name, type, vendor, model, location, firmware, config_version FROM assets";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return assets;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::Asset asset;
        asset.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        asset.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        asset.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        asset.vendor = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        asset.model = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        asset.location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        asset.firmware = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        asset.configurationVersion = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        asset.status = ctm::domain::AssetStatus::OFFLINE; // Base state
        assets.push_back(asset);
    }
    
    sqlite3_finalize(stmt);
    return assets;
}

bool SQLiteRepository::clearAssets() {
    if (!db_) return false;
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, "DELETE FROM assets", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

} // namespace storage
} // namespace ctm
