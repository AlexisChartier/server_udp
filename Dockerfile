FROM debian:bookworm-slim


RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libpqxx-dev \
    libpq-dev \
    libasio-dev \
    pkg-config \
    git \
    && rm -rf /var/lib/apt/lists/*

    WORKDIR /usr/src/app

    COPY . .

    RUN mkdir build && cd build && cmake .. && make
    RUN cmake -s . -B build && cmake --build build

    WORKDIR /usr/src/app/build
    CMD ["./server_udp", "--help"]