cmake_minimum_required(VERSION 3.20)
project(edge-core VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set runtime paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Drogon requires JsonCpp, UUID, zlib, etc. 
# We explicitly require Drogon. If not found, the build will fail gracefully.
find_package(Drogon REQUIRED)
find_package(Jsoncpp REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_executable(edge-core
    src/main.cpp
    src/api/websocket_ctrl.cpp
    src/ingestion/live_state_store.cpp
    src/rules/alarm_engine.cpp
    src/rules/correlation_engine.cpp
    src/rules/energy_engine.cpp
    src/storage/sqlite_repository.cpp
)

target_include_directories(edge-core PRIVATE src)
target_link_libraries(edge-core PRIVATE Drogon::Drogon Jsoncpp_lib unofficial::sqlite3::sqlite3)
