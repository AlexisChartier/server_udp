#pragma once
#include <cstdint>
#include <cstring>
#include <asio.hpp>
#include <chrono>
#include <memory>
#include <iostream>

#include "util/thread_pool.hpp"
#include "net/session.hpp"
#include "db/pg_pipeline.hpp"


namespace sudp::net
{
class UdpServer
{
public:
    UdpServer(uint16_t port, db::PgPipeline& db, std::size_t n_workers = std::thread::hardware_concurrency());

    void run();

    UdpServer(const UdpServer&) = delete;
    UdpServer& operator=(const UdpServer&) = delete;

private:
    void start_receive();

    void db_loop();
    asio::io_context          io_;
    asio::ip::udp::socket     socket_;
    asio::ip::udp::endpoint   remote_;
    std::array<uint8_t, 2048> rx_buf_{};

    BatchQueue<>          queue_;
    db::PgPipeline&       db_;
    util::ThreadPool      pool_;
};
} // namespace sudp::net
