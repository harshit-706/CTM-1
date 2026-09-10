# ============================================================================
# cmake/Sanitizers.cmake
# CTM-1 Edge Core — Runtime sanitizers (ASan, UBSan, TSan, LSan, MSan)
#
# Usage (via cache options or CMakePresets.json):
#   cmake -DCTM_ENABLE_ASAN=ON -DCTM_ENABLE_UBSAN=ON ..
#
# Or use presets:
#   cmake --preset linux-debug-sanitize
# ============================================================================

include_guard(GLOBAL)

# ── Option declarations ────────────────────────────────────────────────────
option(CTM_ENABLE_ASAN  "Enable AddressSanitizer (memory errors)"           OFF)
option(CTM_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer"                 OFF)
option(CTM_ENABLE_TSAN  "Enable ThreadSanitizer (data races)"               OFF)
option(CTM_ENABLE_LSAN  "Enable LeakSanitizer (standalone, no ASan)"        OFF)
option(CTM_ENABLE_MSAN  "Enable MemorySanitizer (uninit reads, Clang only)" OFF)

# ── Mutual exclusion guards ────────────────────────────────────────────────
if(CTM_ENABLE_ASAN AND CTM_ENABLE_TSAN)
    message(FATAL_ERROR "[CTM1] ASan and TSan cannot be enabled simultaneously.")
endif()
if(CTM_ENABLE_MSAN AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "[CTM1] MemorySanitizer requires Clang. Use CTM_ENABLE_MSAN only with Clang.")
endif()
if(CTM_ENABLE_LSAN AND CTM_ENABLE_ASAN)
    message(STATUS "[CTM1] LSan is already bundled with ASan — CTM_ENABLE_LSAN is redundant.")
    set(CTM_ENABLE_LSAN OFF)
endif()

# ── Interface library ──────────────────────────────────────────────────────
add_library(ctm_sanitizer_options INTERFACE)
add_library(ctm::sanitizer_options ALIAS ctm_sanitizer_options)

if(MSVC)
    # MSVC: Only ASan is supported via /fsanitize=address (VS 2019+)
    if(CTM_ENABLE_ASAN)
        target_compile_options(ctm_sanitizer_options INTERFACE /fsanitize=address)
        target_link_options(ctm_sanitizer_options INTERFACE /INFERASANLIBS)
        message(STATUS "[CTM1] Sanitizer: ASAN (MSVC /fsanitize=address)")
    endif()
    if(CTM_ENABLE_UBSAN OR CTM_ENABLE_TSAN OR CTM_ENABLE_MSAN)
        message(WARNING "[CTM1] UBSan/TSan/MSan are not supported on MSVC. Ignored.")
    endif()
    return()
endif()

# ── GCC / Clang (Linux / macOS) ───────────────────────────────────────────
set(_san_compile_flags "")
set(_san_link_flags "")

# ── AddressSanitizer ──────────────────────────────────────────────────────
if(CTM_ENABLE_ASAN)
    list(APPEND _san_compile_flags
        -fsanitize=address
        -fno-omit-frame-pointer        # Better stack traces
        -fno-optimize-sibling-calls    # Accurate call stacks
    )
    list(APPEND _san_link_flags -fsanitize=address)
    message(STATUS "[CTM1] Sanitizer: ASan ENABLED")
    message(STATUS "       Runtime options: ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1")
endif()

# ── UndefinedBehaviorSanitizer ────────────────────────────────────────────
if(CTM_ENABLE_UBSAN)
    list(APPEND _san_compile_flags
        -fsanitize=undefined
        -fsanitize=bounds
        -fsanitize=null
        -fsanitize=return
        -fsanitize=signed-integer-overflow
        -fsanitize=shift
        -fsanitize=integer-divide-by-zero
        -fsanitize=unreachable
        -fsanitize=float-cast-overflow
        -fsanitize=float-divide-by-zero
        -fno-sanitize-recover=all      # Abort on first error (don't continue)
    )
    list(APPEND _san_link_flags -fsanitize=undefined)
    message(STATUS "[CTM1] Sanitizer: UBSan ENABLED (strict — abort on first error)")
endif()

# ── ThreadSanitizer ───────────────────────────────────────────────────────
if(CTM_ENABLE_TSAN)
    list(APPEND _san_compile_flags
        -fsanitize=thread
        -fno-omit-frame-pointer
    )
    list(APPEND _san_link_flags -fsanitize=thread)
    message(STATUS "[CTM1] Sanitizer: TSan ENABLED — use with single-threaded init only")
    message(STATUS "       Runtime options: TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1")
endif()

# ── LeakSanitizer (standalone) ────────────────────────────────────────────
if(CTM_ENABLE_LSAN)
    list(APPEND _san_compile_flags -fsanitize=leak)
    list(APPEND _san_link_flags -fsanitize=leak)
    message(STATUS "[CTM1] Sanitizer: LSan ENABLED (standalone)")
endif()

# ── MemorySanitizer (Clang only) ──────────────────────────────────────────
if(CTM_ENABLE_MSAN)
    list(APPEND _san_compile_flags
        -fsanitize=memory
        -fsanitize-memory-track-origins=2
        -fno-omit-frame-pointer
    )
    list(APPEND _san_link_flags -fsanitize=memory)
    message(STATUS "[CTM1] Sanitizer: MSan ENABLED (Clang only, uninit read detection)")
endif()

# ── Apply flags if any sanitizer enabled ──────────────────────────────────
if(_san_compile_flags)
    # Disable optimizations that interfere with sanitizer accuracy
    list(APPEND _san_compile_flags -O1 -g)

    target_compile_options(ctm_sanitizer_options INTERFACE ${_san_compile_flags})
    target_link_options(ctm_sanitizer_options INTERFACE ${_san_link_flags})

    # Expose runtime env hints as CMake properties
    set_target_properties(ctm_sanitizer_options PROPERTIES
        INTERFACE_CTM_ASAN_ENABLED  "${CTM_ENABLE_ASAN}"
        INTERFACE_CTM_UBSAN_ENABLED "${CTM_ENABLE_UBSAN}"
        INTERFACE_CTM_TSAN_ENABLED  "${CTM_ENABLE_TSAN}"
    )
else()
    message(STATUS "[CTM1] Sanitizers: DISABLED (Release mode)")
endif()
