FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libpcap-dev \
    nlohmann-json3-dev \
    libboost-system-dev \
    libboost-filesystem-dev \
    libspdlog-dev \
    libfmt-dev \
    libssl-dev \
    git

WORKDIR /app
COPY . .

RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make -j$(nproc)

FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y \
    libpcap0.8 \
    libboost-system1.74.0 \
    libboost-filesystem1.74.0 \
    libssl3 \
    libfmt8 \
    iproute2 \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/netsentry /app/
COPY configs /app/configs

EXPOSE 8080 9090

ENTRYPOINT ["./netsentry", "--api-enable", "--config", "/app/configs/netsentry.yaml"]
