#pragma once
#include <cstdint>
#include <cstring>
#include <asio.hpp>
#include <chrono>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <array>
#include <mutex>

#include "util/thread_pool.hpp"
#include "core/reassembly_buffer.hpp"
#include "core/udp_header.hpp"
#include "net/db_queue.hpp"

namespace sudp::net
{
class UdpServer
{
public:
    UdpServer(uint16_t port, DbQueue& queue, std::size_t n_workers = std::thread::hardware_concurrency());

    void run();

    UdpServer(const UdpServer&) = delete;
    UdpServer& operator=(const UdpServer&) = delete;

private:
    void start_receive();
    void handle_packet(const uint8_t* data, std::size_t len);

    asio::io_context          io_;
    asio::ip::udp::socket     socket_;
    asio::ip::udp::endpoint   remote_;
    std::array<uint8_t, 2048> rx_buf_{};

    DbQueue& queue_;
    util::ThreadPool pool_;

    struct FragKey {
        uint8_t  drone_id;
        uint32_t seq;
        bool operator==(const FragKey& other) const noexcept {
            return drone_id == other.drone_id && seq == other.seq;
        }
    };

    struct FragKeyHash {
        std::size_t operator()(const FragKey& k) const noexcept {
            return (std::hash<uint8_t>{}(k.drone_id) << 1) ^ std::hash<uint32_t>{}(k.seq);
        }
    };

    std::unordered_map<FragKey, core::ReassemblyBuffer, FragKeyHash> buffers_;
    std::mutex mutex_;
};
} // namespace sudp::net

