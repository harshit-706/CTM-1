#pragma once
#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>
#include "../domain/types.h"

namespace ctm {
namespace storage {

class SQLiteRepository {
public:
    static SQLiteRepository& getInstance();

    bool initialize(const std::string& dbPath);

    // Audit operations
    bool logAuditEvent(const std::string& action, const std::string& target, const std::string& details);

    // Metadata operations
    bool saveAsset(const domain::Asset& asset);
    std::optional<domain::Asset> getAsset(const std::string& id);
    std::vector<domain::Asset> getAllAssets();
    bool clearAssets();

private:
    SQLiteRepository() = default;
    ~SQLiteRepository();
    
    sqlite3* db_ = nullptr;
};

} // namespace storage
} // namespace ctm
