# ============================================================================
# cmake/Dependencies.cmake
# CTM-1 Edge Core — Dependency resolution via vcpkg manifest mode.
#
# vcpkg manifest (vcpkg.json) must exist at project root.
# VCPKG_ROOT or CMAKE_TOOLCHAIN_FILE must be set, or vcpkg auto-detected.
# ============================================================================

include_guard(GLOBAL)

# ── vcpkg toolchain auto-detection ────────────────────────────────────────
# Priority: explicit CMAKE_TOOLCHAIN_FILE > VCPKG_ROOT env > common paths
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(_vcpkg_search_paths
        "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "/opt/vcpkg/scripts/buildsystems/vcpkg.cmake"
    )
    foreach(_p IN LISTS _vcpkg_search_paths)
        if(EXISTS "${_p}")
            set(CMAKE_TOOLCHAIN_FILE "${_p}" CACHE STRING "vcpkg toolchain")
            message(STATUS "[CTM1] vcpkg toolchain: ${_p}")
            break()
        endif()
    endforeach()
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(WARNING "[CTM1] vcpkg toolchain NOT found. "
            "Set VCPKG_ROOT environment variable or -DCMAKE_TOOLCHAIN_FILE=<path>")
    endif()
endif()

# ── vcpkg manifest mode ────────────────────────────────────────────────────
# vcpkg.json at project root drives dependency installation automatically.
set(VCPKG_MANIFEST_MODE ON)

# ── Drogon (HTTP/WebSocket framework) ─────────────────────────────────────
find_package(Drogon REQUIRED)
message(STATUS "[CTM1] Drogon: ${Drogon_VERSION}")

# ── JsonCpp ───────────────────────────────────────────────────────────────
find_package(jsoncpp CONFIG REQUIRED)
# Drogon re-exports jsoncpp; also get it directly for own use
if(TARGET JsonCpp::JsonCpp)
    set(JSONCPP_TARGET JsonCpp::JsonCpp)
elseif(TARGET jsoncpp_lib)
    set(JSONCPP_TARGET jsoncpp_lib)
else()
    message(FATAL_ERROR "[CTM1] JsonCpp target not found after find_package.")
endif()
message(STATUS "[CTM1] JsonCpp target: ${JSONCPP_TARGET}")

# ── SQLite3 ───────────────────────────────────────────────────────────────
find_package(unofficial-sqlite3 CONFIG REQUIRED)
message(STATUS "[CTM1] SQLite3: found via unofficial-sqlite3")

# ── libmodbus (Modbus TCP/RTU) ─────────────────────────────────────────────
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(MODBUS QUIET libmodbus)
endif()

if(NOT MODBUS_FOUND)
    # Fallback: try vcpkg-installed modbus header/lib
    find_path(MODBUS_INCLUDE_DIR modbus/modbus.h HINTS "${VCPKG_INSTALLED_DIR}")
    find_library(MODBUS_LIBRARY NAMES modbus libmodbus HINTS "${VCPKG_INSTALLED_DIR}")
    if(MODBUS_INCLUDE_DIR AND MODBUS_LIBRARY)
        set(MODBUS_FOUND TRUE)
        set(MODBUS_INCLUDE_DIRS "${MODBUS_INCLUDE_DIR}")
        set(MODBUS_LIBRARIES "${MODBUS_LIBRARY}")
    endif()
endif()

if(NOT MODBUS_FOUND)
    # Last resort: system libmodbus (Ubuntu: apt install libmodbus-dev)
    find_path(MODBUS_INCLUDE_DIR modbus/modbus.h
        PATHS /usr/include /usr/local/include)
    find_library(MODBUS_LIBRARY NAMES modbus
        PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
    if(MODBUS_INCLUDE_DIR AND MODBUS_LIBRARY)
        set(MODBUS_FOUND TRUE)
        set(MODBUS_INCLUDE_DIRS "${MODBUS_INCLUDE_DIR}")
        set(MODBUS_LIBRARIES "${MODBUS_LIBRARY}")
        message(STATUS "[CTM1] libmodbus: found at system path (${MODBUS_LIBRARY})")
    endif()
endif()

if(MODBUS_FOUND)
    # Create an imported target for clean linking
    if(NOT TARGET modbus::modbus)
        add_library(modbus::modbus UNKNOWN IMPORTED)
        set_target_properties(modbus::modbus PROPERTIES
            IMPORTED_LOCATION             "${MODBUS_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${MODBUS_INCLUDE_DIRS}"
        )
    endif()
    message(STATUS "[CTM1] Modbus TCP: ENABLED (${MODBUS_LIBRARIES})")
    set(CTM_MODBUS_AVAILABLE TRUE CACHE BOOL "Modbus support available" FORCE)
else()
    message(WARNING "[CTM1] libmodbus NOT found. Modbus TCP support DISABLED.\n"
        "  Ubuntu install: sudo apt install libmodbus-dev\n"
        "  vcpkg install:  vcpkg install libmodbus")
    set(CTM_MODBUS_AVAILABLE FALSE CACHE BOOL "Modbus support available" FORCE)
endif()

# ── Threads (pthreads on Linux) ───────────────────────────────────────────
find_package(Threads REQUIRED)
message(STATUS "[CTM1] Threads: ${CMAKE_THREAD_LIBS_INIT}")

# ── Summary ───────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "┌─ CTM-1 Dependency Summary ─────────────────────")
message(STATUS "│  Drogon    : ${Drogon_VERSION}")
message(STATUS "│  JsonCpp   : ${JSONCPP_TARGET}")
message(STATUS "│  SQLite3   : unofficial-sqlite3")
message(STATUS "│  Modbus    : ${CTM_MODBUS_AVAILABLE}")
message(STATUS "│  Threads   : ${CMAKE_THREAD_LIBS_INIT}")
message(STATUS "└────────────────────────────────────────────────")
message(STATUS "")
