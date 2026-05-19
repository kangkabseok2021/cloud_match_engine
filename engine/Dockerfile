FROM debian:bookworm-slim AS builder
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake ninja-build g++ libgrpc++-dev protobuf-compiler-grpc \
    libprotobuf-dev ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
COPY ../proto /proto

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja \
    && cmake --build build --target match_engine

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    libgrpc++1 libprotobuf32 && rm -rf /var/lib/apt/lists/*

RUN useradd --system --no-create-home engine
COPY --from=builder /src/build/match_engine /usr/local/bin/match_engine
USER engine
EXPOSE 50051
CMD ["match_engine", "0.0.0.0:50051"]
