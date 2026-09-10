# ============================================================================
# CTM-1 Edge Core — Root CMakeLists.txt
# Build System: CMake 4.0 | Target OS: Ubuntu 24.04 LTS (Noble)
# Toolchain: GCC 13 / Clang 17 | C++ Standard: C++20
#
# Quick start (Linux):
#   cmake --preset linux-debug-sanitize
#   cmake --build --preset linux-debug-sanitize
#
# Quick start (Docker):
#   docker build -t ctm1-edge:latest .
# ============================================================================

cmake_minimum_required(VERSION 3.25...4.0)

# ── Project declaration ────────────────────────────────────────────────────
project(edge-core
    VERSION      1.0.0
    DESCRIPTION  "CTM-1 Industrial Edge Core — HTTP/WebSocket/Modbus Gateway"
    HOMEPAGE_URL "https://github.com/hedoescode-mayank/CTM-1"
    LANGUAGES    CXX
)

# ── Guard: out-of-source builds only ──────────────────────────────────────
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "[CTM1] In-source builds are not allowed.\n"
        "  Run: cmake -S . -B build/<preset>\n"
        "  Or:  cmake --preset linux-debug-sanitize")
endif()

# ── cmake/ module path ─────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── Global build settings ──────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)    # Use -std=c++20, not gnu++20

# Export compile commands (clangd, clang-tidy, IDE support)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

message(STATUS "")
message(STATUS "┌─ CTM-1 Edge Core Build ─────────────────────────────────")
message(STATUS "│  Version    : ${PROJECT_VERSION}")
message(STATUS "│  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "│  CMake      : ${CMAKE_VERSION}")
message(STATUS "│  OS         : ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "└─────────────────────────────────────────────────────────")
message(STATUS "")

# ── Load cmake modules (order matters) ─────────────────────────────────────
include(cmake/CompilerOptions.cmake)   # Must come first — defines ctm::compiler_options
include(cmake/Sanitizers.cmake)        # Defines ctm::sanitizer_options
include(cmake/Dependencies.cmake)      # Resolves all external dependencies
include(cmake/Targets.cmake)           # Defines all library and executable targets
                                       # NO file(GLOB) anywhere in this build
