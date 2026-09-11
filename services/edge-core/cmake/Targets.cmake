# ============================================================================
# cmake/Targets.cmake
# CTM-1 Edge Core — Explicit target definitions. NO file(GLOB ...) anywhere.
#
# Every source file is listed explicitly so:
#  - CMake re-runs correctly when files are added/removed
#  - No hidden/stale object files enter the build
#  - CI diffs are clean and auditable
# ============================================================================

include_guard(GLOBAL)

# ── Module: domain ─────────────────────────────────────────────────────────
# Pure header-only domain types. No .cpp needed.
add_library(ctm_domain INTERFACE)
add_library(ctm::domain ALIAS ctm_domain)
target_include_directories(ctm_domain INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# ── Module: storage ────────────────────────────────────────────────────────
add_library(ctm_storage STATIC
    src/storage/sqlite_repository.cpp
)
add_library(ctm::storage ALIAS ctm_storage)
target_include_directories(ctm_storage PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_link_libraries(ctm_storage
    PUBLIC  ctm::domain
    PRIVATE ctm::compiler_options
            ctm::sanitizer_options
            unofficial::sqlite3::sqlite3
            Threads::Threads
)

# ── Module: ingestion ──────────────────────────────────────────────────────
add_library(ctm_ingestion STATIC
    src/ingestion/live_state_store.cpp
)
add_library(ctm::ingestion ALIAS ctm_ingestion)
target_include_directories(ctm_ingestion PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_link_libraries(ctm_ingestion
    PUBLIC  ctm::domain
    PRIVATE ctm::compiler_options
            ctm::sanitizer_options
            Threads::Threads
)

# ── Module: rules ──────────────────────────────────────────────────────────
add_library(ctm_rules STATIC
    src/rules/alarm_engine.cpp
    src/rules/correlation_engine.cpp
    src/rules/energy_engine.cpp
)
add_library(ctm::rules ALIAS ctm_rules)
target_include_directories(ctm_rules PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_link_libraries(ctm_rules
    PUBLIC  ctm::domain
            ctm::ingestion
    PRIVATE ctm::compiler_options
            ctm::sanitizer_options
            Drogon::Drogon
            Threads::Threads
)

# ── Module: api ────────────────────────────────────────────────────────────
add_library(ctm_api OBJECT
    src/api/rest_ctrl.cpp
    src/api/topology_ctrl.cpp
    src/api/websocket_ctrl.cpp
)
add_library(ctm::api ALIAS ctm_api)
target_include_directories(ctm_api PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_link_libraries(ctm_api
    PUBLIC  ctm::domain
            ctm::ingestion
            ctm::rules
            ctm::storage
    PRIVATE ctm::compiler_options
            ctm::sanitizer_options
            Drogon::Drogon
            ${JSONCPP_TARGET}
            Threads::Threads
)

# ── Module: modbus (conditional) ───────────────────────────────────────────
if(CTM_MODBUS_AVAILABLE)
    add_library(ctm_modbus STATIC
        src/modbus/modbus_server.cpp
    )
    add_library(ctm::modbus ALIAS ctm_modbus)
    target_include_directories(ctm_modbus PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    )
    target_compile_definitions(ctm_modbus PUBLIC CTM_MODBUS_ENABLED=1)
    target_link_libraries(ctm_modbus
        PUBLIC  ctm::domain
                ctm::ingestion
        PRIVATE ctm::compiler_options
                ctm::sanitizer_options
                modbus::modbus
                Threads::Threads
    )
else()
    # Stub target so ctm::modbus is always linkable
    add_library(ctm_modbus INTERFACE)
    add_library(ctm::modbus ALIAS ctm_modbus)
    target_compile_definitions(ctm_modbus INTERFACE CTM_MODBUS_ENABLED=0)
    message(STATUS "[CTM1] ctm::modbus stub created (Modbus disabled)")
endif()

# ── Main executable ────────────────────────────────────────────────────────
add_executable(edge-core
    src/main.cpp
)
target_include_directories(edge-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(edge-core
    PRIVATE ctm::compiler_options
            ctm::sanitizer_options
            ctm::api
            ctm::modbus
            ctm::storage
            ctm::ingestion
            ctm::rules
            ctm::domain
            Drogon::Drogon
            ${JSONCPP_TARGET}
            unofficial::sqlite3::sqlite3
            Threads::Threads
)

# ── Output paths ───────────────────────────────────────────────────────────
set_target_properties(edge-core PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    DEBUG_POSTFIX            "-debug"
)

# ── Install rules (for Docker COPY) ───────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS edge-core
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ── Compile-time asset version string ─────────────────────────────────────
target_compile_definitions(edge-core PRIVATE
    CTM1_VERSION_MAJOR=1
    CTM1_VERSION_MINOR=0
    CTM1_VERSION_PATCH=0
    CTM1_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
)
