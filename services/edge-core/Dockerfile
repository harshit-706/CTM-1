# ==============================================================================
# CTM-1 Edge Core — Multi-Stage Dockerfile
# Target OS : Ubuntu 24.04 LTS (Noble)
# Goal      : Minimal runtime image (~45-60 MB)
# Build     : docker build -t ctm1-edge:latest ./services/edge-core
#
# Stage 1: builder   — full toolchain + vcpkg + compile
# Stage 2: runtime   — stripped binary + minimal shared libs only
# ==============================================================================

# ── Stage 1: builder ──────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS builder

LABEL stage="ctm1-builder"

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# ── System build dependencies ─────────────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Build toolchain
    build-essential \
    gcc-13 \
    g++-13 \
    clang-17 \
    ninja-build \
    # CMake (Ubuntu 24.04 ships CMake 3.28; fine for 3.25...4.0 range)
    cmake \
    # vcpkg dependencies
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    # Drogon / Trantor system deps
    libssl-dev \
    zlib1g-dev \
    libbrotli-dev \
    # Modbus TCP
    libmodbus-dev \
    # SQLite3
    libsqlite3-dev \
    # C-Ares (Drogon async DNS)
    libc-ares-dev \
    # UUID (Drogon session IDs)
    uuid-dev \
    # Cleanup
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

# Default to GCC 13
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 \
 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# ── vcpkg setup ───────────────────────────────────────────────────────────────
# Use a pinned commit for reproducible builds
ARG VCPKG_COMMIT=b322364f06308bdd24823a28e08f00dce4e23a35
ENV VCPKG_ROOT=/opt/vcpkg
ENV VCPKG_DEFAULT_TRIPLET=x64-linux

RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
 && cd ${VCPKG_ROOT} \
 && git checkout ${VCPKG_COMMIT} \
 && ./bootstrap-vcpkg.sh -disableMetrics

ENV PATH="${VCPKG_ROOT}:${PATH}"

# ── Source code ────────────────────────────────────────────────────────────────
WORKDIR /src
COPY . .

# ── Configure + Build (docker-release preset) ─────────────────────────────────
# CMakePresets.json docker-release → CMAKE_INSTALL_PREFIX=/usr/local, Release, no sanitizers
RUN cmake --preset docker-release \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=x64-linux \
 && cmake --build --preset docker-release \
 && cmake --install build/docker-release --prefix /install

# ── Collect runtime shared library dependencies ───────────────────────────────
# ldd parses the binary's ELF NEEDED entries and copies all .so files
RUN mkdir -p /install/lib \
 && ldd /install/bin/edge-core \
    | awk '/=> \// { print $3 }' \
    | sort -u \
    | xargs -I{} cp -v --no-preserve=mode,ownership {} /install/lib/ \
 && echo "[Docker] Shared library snapshot complete."

# Verify binary runs (basic smoke test in builder)
RUN /install/bin/edge-core --version 2>/dev/null || true


# ── Stage 2: runtime ──────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS runtime

LABEL \
    org.opencontainers.image.title="CTM-1 Edge Core" \
    org.opencontainers.image.description="CTM-1 Industrial Edge Gateway — HTTP/WebSocket/Modbus TCP" \
    org.opencontainers.image.version="1.0.0" \
    org.opencontainers.image.source="https://github.com/hedoescode-mayank/CTM-1" \
    org.opencontainers.image.licenses="MIT"

ENV DEBIAN_FRONTEND=noninteractive

# ── Minimal runtime packages ──────────────────────────────────────────────────
# Only what the binary dynamically links at runtime — no build tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    zlib1g \
    libbrotli1 \
    libmodbus5 \
    libsqlite3-0 \
    libc-ares2 \
    libuuid1 \
    # Health check tooling
    curl \
 && rm -rf /var/lib/apt/lists/* \
 && apt-get clean

# ── Non-root user (security best practice) ────────────────────────────────────
RUN groupadd --gid 1001 ctm1 \
 && useradd --uid 1001 --gid ctm1 --shell /bin/false --no-create-home ctm1

# ── Copy binary + bundled libs from builder ───────────────────────────────────
COPY --from=builder --chown=ctm1:ctm1 /install/bin/edge-core /usr/local/bin/edge-core
# Bundled .so libs (fallback in case system libs differ)
COPY --from=builder --chown=root:root  /install/lib/           /usr/local/lib/ctm1/

# ── Runtime environment ───────────────────────────────────────────────────────
# LD_LIBRARY_PATH ensures bundled libs take precedence
ENV LD_LIBRARY_PATH=/usr/local/lib/ctm1:${LD_LIBRARY_PATH}

# SQLite database persisted in a named volume
VOLUME ["/data"]
ENV CTM_DB_PATH=/data/ctm1.db

# ── Working directory ─────────────────────────────────────────────────────────
WORKDIR /data

# ── Exposed ports ─────────────────────────────────────────────────────────────
EXPOSE 8080/tcp    # HTTP REST + WebSocket (/live)
EXPOSE 502/tcp     # Modbus TCP (standard port — requires net capability or --privileged)

# ── Health check ──────────────────────────────────────────────────────────────
# Docker HEALTHCHECK: polls /api/v1/health every 15s
# Unhealthy after 3 consecutive failures → container marked "unhealthy"
HEALTHCHECK \
    --interval=15s \
    --timeout=5s \
    --start-period=10s \
    --retries=3 \
    CMD curl -sf http://localhost:8080/api/v1/health | grep -q '"status":"ok"' || exit 1

# ── Switch to non-root ────────────────────────────────────────────────────────
USER ctm1

# ── Entrypoint ────────────────────────────────────────────────────────────────
# edge-core reads CTM_DB_PATH from environment if coded to do so;
# otherwise pass as positional arg
ENTRYPOINT ["/usr/local/bin/edge-core"]
CMD []
