# ──────────────────────────────────────────────────────────────────────────────
# Stage 1 – Build the LLVM passes + Go orchestrator + API server
# ──────────────────────────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS builder

# Avoid interactive prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install build deps: LLVM/Clang 18, CMake, Go, and general utilities
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        clang-18 \
        llvm-18-dev \
        llvm-18-tools \
        lld-18 \
        libclang-18-dev \
        golang-go \
        git \
        ca-certificates \
        python3 \
    && ln -sf /usr/bin/clang-18   /usr/local/bin/clang \
    && ln -sf /usr/bin/clang++-18 /usr/local/bin/clang++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy the full project source
COPY . /build/

# ── 1a. Build the LLVM C++ passes ─────────────────────────────────────────────
RUN mkdir -p /build/build \
    && cd /build/build \
    && cmake \
        -DLLVM_DIR=/usr/lib/llvm-18/cmake \
        -DCMAKE_BUILD_TYPE=Release \
        .. \
    && make -j"$(nproc)"

# ── 1b. Build the Go orchestrator (compiler_wrapper) ──────────────────────────
RUN cd /build/orchestrator \
    && go mod tidy \
    && go build -o /build/build/compiler_wrapper ./cmd/compiler_wrapper.go

# Move the .so plugins next to compiler_wrapper so the wrapper can locate them
RUN mkdir -p /build/build/plugins \
    && find /build/build -maxdepth 1 -name "*.so" -exec mv {} /build/build/plugins/ \;

# ── 1c. Build the REST API server ─────────────────────────────────────────────
RUN cd /build/api \
    && go mod tidy \
    && CGO_ENABLED=0 go build -o /build/build/hideir-api .

# ──────────────────────────────────────────────────────────────────────────────
# Stage 2 – Lean runtime image
# ──────────────────────────────────────────────────────────────────────────────
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Runtime deps only: clang (to actually compile user code) + binutils for strip/strings/nm
RUN apt-get update && apt-get install -y --no-install-recommends \
        clang-18 \
        llvm-18 \
        binutils \
        libstdc++6 \
        ca-certificates \
    && ln -sf /usr/bin/clang-18   /usr/local/bin/clang \
    && ln -sf /usr/bin/clang++-18 /usr/local/bin/clang++ \
    && rm -rf /var/lib/apt/lists/*

# Gather everything under /hideir
WORKDIR /hideir

# Binaries & plugins from the build stage
COPY --from=builder /build/build/compiler_wrapper  /hideir/build/compiler_wrapper
COPY --from=builder /build/build/plugins/          /hideir/build/plugins/
COPY --from=builder /build/build/hideir-api        /hideir/hideir-api

# Shell wrapper script + configs needed at runtime
COPY --from=builder /build/hideir.sh               /hideir/hideir.sh
COPY --from=builder /build/orchestrator/config/    /hideir/orchestrator/config/

RUN chmod +x /hideir/hideir.sh /hideir/build/compiler_wrapper /hideir/hideir-api

# ── Environment variables consumed by hideir.sh and the API ───────────────────
ENV HIDEIR_WRAPPER=/hideir/hideir.sh
ENV HIDEIR_DEFAULT_CONFIG=/hideir/orchestrator/config/default_config.yaml
# Let the wrapper find the pre-built compiler_wrapper binary
ENV PATH="/hideir/build:${PATH}"

# ── Expose API port ───────────────────────────────────────────────────────────
EXPOSE 8080

# ── Health-check ──────────────────────────────────────────────────────────────
HEALTHCHECK --interval=15s --timeout=5s --start-period=10s \
    CMD wget -qO- http://localhost:8080/health || exit 1

ENTRYPOINT ["/hideir/hideir-api"]
