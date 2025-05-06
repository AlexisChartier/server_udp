FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libpq-dev \
    pkg-config \
    git \
    g++ \
    libssl-dev \
    zlib1g-dev \
    wget

# Installer libpqxx en source
RUN cd /tmp && \
    wget https://github.com/jtv/libpqxx/archive/refs/tags/7.7.5.tar.gz && \
    tar xvf 7.7.5.tar.gz && \
    cd libpqxx-7.7.5 && \
    cmake -S . -B build -DCMAKE_CXX_STANDARD=20 && \
    cmake --build build && \
    cmake --install build

WORKDIR /usr/src/app
COPY . .

RUN mkdir build && cd build && cmake .. && make

WORKDIR /usr/src/app/build
CMD ["./server_udp", "--help"]

