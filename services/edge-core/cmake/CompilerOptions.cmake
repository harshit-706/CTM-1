# ============================================================================
# cmake/CompilerOptions.cmake
# CTM-1 Edge Core — Compiler hardening, warnings, and C++20 enforcement.
# Supports: GCC >= 11, Clang >= 14, MSVC >= 19.29 (VS 2019 16.10+)
# ============================================================================

include_guard(GLOBAL)

# ----------------------------------------------------------------------------
# Interface library: apply options to any target via target_link_libraries
# ----------------------------------------------------------------------------
add_library(ctm_compiler_options INTERFACE)
add_library(ctm::compiler_options ALIAS ctm_compiler_options)

# ── C++ Standard ──────────────────────────────────────────────────────────
target_compile_features(ctm_compiler_options INTERFACE cxx_std_20)
set_target_properties(ctm_compiler_options PROPERTIES
    INTERFACE_CXX_EXTENSIONS OFF          # -std=c++20, not gnu++20
)

# ── Compiler-specific flags ───────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

    target_compile_options(ctm_compiler_options INTERFACE
        # ── Diagnostic basics
        -Wall
        -Wextra
        -Wpedantic

        # ── Pointer / null safety
        -Wnull-dereference
        -Wdangling-else

        # ── Shadowing & scope
        -Wshadow

        # ── Format string attacks
        -Wformat=2
        -Wformat-security

        # ── Integer / float precision
        -Wdouble-promotion           # float silently promoted to double
        -Wfloat-conversion
        -Wsign-conversion
        -Wconversion

        # ── Unreachable / missing return
        -Wunreachable-code
        -Wreturn-type

        # ── Unused symbols
        -Wunused-variable
        -Wunused-parameter
        -Wunused-function

        # ── C++20 specific
        -Wno-deprecated-declarations  # silence Drogon internal deprecations

        # ── Hardening (stack/GOT)
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2

        # ── Position-independent code (required for shared libs & ASLR)
        -fPIC

        # ── Linker hardening (full RELRO)
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-pie>
    )

    # Clang-only extras
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(ctm_compiler_options INTERFACE
            -Wno-c99-extensions
            -Wthread-safety             # Clang thread-safety annotations
        )
    endif()

    # GCC-only extras
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(ctm_compiler_options INTERFACE
            -Wlogical-op               # suspicious logical operators
            -Wduplicated-cond          # if (a) ... else if (a) ...
            -Wmisleading-indentation
        )
    endif()

elseif(MSVC)

    target_compile_options(ctm_compiler_options INTERFACE
        /W4                     # Highest practical warning level
        /WX                     # Warnings as errors
        /permissive-            # Strict standards conformance
        /w14640                 # Thread-unsafe static member init
        /w14826                 # Sign extension
        /w14928                 # Illegal copy-initialization
        /wd4068                 # Unknown pragma (suppress for #pragma once etc.)
        /MP                     # Multi-processor compilation
        /Zc:__cplusplus         # Correct __cplusplus macro value
        /Zc:preprocessor        # Standards-conforming preprocessor
    )

    # Security flags for MSVC
    target_compile_definitions(ctm_compiler_options INTERFACE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS  # Drogon uses some legacy C APIs
    )

endif()

# ── Build-type specific optimizations ─────────────────────────────────────
target_compile_options(ctm_compiler_options INTERFACE
    $<$<CONFIG:Release>:$<IF:$<CXX_COMPILER_ID:MSVC>,/O2 /GL,-O3 -march=native>>
    $<$<CONFIG:Debug>:$<IF:$<CXX_COMPILER_ID:MSVC>,/Od /RTC1,-O0 -g3>>
    $<$<CONFIG:RelWithDebInfo>:$<IF:$<CXX_COMPILER_ID:MSVC>,/O2 /Zi,-O2 -g>>
)

# ── LTO for Release ────────────────────────────────────────────────────────
include(CheckIPOSupported)
check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_reason)
if(_ipo_ok)
    target_compile_options(ctm_compiler_options INTERFACE
        $<$<CONFIG:Release>:$<IF:$<CXX_COMPILER_ID:MSVC>,/GL,-flto=auto>>
    )
    message(STATUS "[CTM1] LTO/IPO: supported (enabled for Release builds)")
else()
    message(STATUS "[CTM1] LTO/IPO: not supported — ${_ipo_reason}")
endif()

message(STATUS "[CTM1] Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "[CTM1] C++ Standard: C++20 (extensions OFF)")
