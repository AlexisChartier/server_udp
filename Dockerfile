FROM ubuntu:22.04

RUN sed -i 's|http://archive.ubuntu.com|http://security.ubuntu.com|g' /etc/apt/sources.list

RUN apt-get update && apt-get install -y  g++\
    build-essential \
    cmake \
    libpq-dev \
    pkg-config \
    git \
    g++ \
    libssl-dev \
    zlib1g-dev \
    wget


WORKDIR /usr/src/app
COPY . .

RUN mkdir build && cd build && cmake .. && make

WORKDIR /usr/src/app/build
CMD ["./server_udp", "--help"]

