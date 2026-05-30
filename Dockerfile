# ──────────────────────────────────────────────────────────────────────────────
# Stage 1 – Build LLVM passes + Go orchestrator + API server
# ──────────────────────────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Build deps: LLVM/Clang 18, CMake, curl (for Go download), build essentials
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        clang-18 \
        llvm-18-dev \
        llvm-18-tools \
        lld-18 \
        libclang-18-dev \
        curl \
        ca-certificates \
        python3 \
    && ln -sf /usr/bin/clang-18   /usr/local/bin/clang \
    && ln -sf /usr/bin/clang++-18 /usr/local/bin/clang++ \
    && rm -rf /var/lib/apt/lists/*

# Install Go 1.22 (distro golang-go may be older)
RUN curl -fsSL https://go.dev/dl/go1.22.4.linux-amd64.tar.gz \
    | tar -C /usr/local -xz
ENV PATH="/usr/local/go/bin:${PATH}"

WORKDIR /build

# Copy full project
COPY . /build/

# ── 1a. Build LLVM C++ passes ─────────────────────────────────────────────────
RUN mkdir -p /build/build \
    && cd /build/build \
    && cmake \
        -DLLVM_DIR=/usr/lib/llvm-18/cmake \
        -DCMAKE_BUILD_TYPE=Release \
        .. \
    && make -j"$(nproc)"

# Move .so plugins into a dedicated subdirectory
RUN mkdir -p /build/build/plugins \
    && find /build/build -maxdepth 1 -name "*.so" -exec mv {} /build/build/plugins/ \;

# ── 1b. Build Go orchestrator (compiler_wrapper) ──────────────────────────────
RUN cd /build/orchestrator \
    && go mod tidy \
    && go build -o /build/build/compiler_wrapper ./cmd/compiler_wrapper.go

# ── 1c. Build REST API server ─────────────────────────────────────────────────
RUN cd /build/api \
    && go mod tidy \
    && CGO_ENABLED=0 GOOS=linux go build -o /build/build/hideir-api .

# ──────────────────────────────────────────────────────────────────────────────
# Stage 2 – Lean runtime image
# ──────────────────────────────────────────────────────────────────────────────
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        clang-18 \
        llvm-18 \
        binutils \
        libstdc++6 \
        ca-certificates \
        wget \
    && ln -sf /usr/bin/clang-18   /usr/local/bin/clang \
    && ln -sf /usr/bin/clang++-18 /usr/local/bin/clang++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /hideir

# Artifacts from build stage
COPY --from=builder /build/build/compiler_wrapper  /hideir/build/compiler_wrapper
COPY --from=builder /build/build/plugins/          /hideir/build/plugins/
COPY --from=builder /build/build/hideir-api        /hideir/hideir-api

# Shell wrapper + configs
COPY --from=builder /build/hideir.sh               /hideir/hideir.sh
COPY --from=builder /build/orchestrator/config/    /hideir/orchestrator/config/

RUN chmod +x /hideir/hideir.sh /hideir/build/compiler_wrapper /hideir/hideir-api

# Let hideir.sh find the pre-built compiler_wrapper
ENV PATH="/hideir/build:${PATH}"
ENV HIDEIR_WRAPPER=/hideir/hideir.sh
ENV HIDEIR_DEFAULT_CONFIG=/hideir/orchestrator/config/default_config.yaml

EXPOSE 8080

HEALTHCHECK --interval=15s --timeout=5s --start-period=10s \
    CMD wget -qO- http://localhost:8080/health || exit 1

ENTRYPOINT ["/hideir/hideir-api"]
