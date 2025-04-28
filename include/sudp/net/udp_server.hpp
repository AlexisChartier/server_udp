#pragma once
#include "util/thread_pool.hpp"
#include "net/session.hpp"
#include "db/pg_pipeline.hpp"
#include <asio.hpp>
#include <memory>
#include <iostream>

namespace sudp::net
{
class UdpServer
{
public:
    UdpServer(uint16_t port,
              db::PgPipeline& db,
              std::size_t n_workers = std::thread::hardware_concurrency())
    : socket_{io_, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)}
    , db_{db}
    , pool_{n_workers}
    {
        start_receive();
        // worker BD : consomme la queue et écrit en base
        pool_.post([this]{ db_loop(); });
    }

    void run() { io_.run(); }

private:
    void start_receive()
    {
        socket_.async_receive_from(
            asio::buffer(rx_buf_), remote_,
            [this](auto ec, std::size_t len)
            {
                if (!ec && len > 0) {
                    // déléguer le parsing au pool
                    pool_.post([this,len]{
                        Session{rx_buf_.data(), len, queue_};
                    });
                }
                start_receive();
            });
    }

    void db_loop()
    {
        std::vector<common_dmw::Voxel> tmp;
        tmp.reserve(2048);
        while(true)
        {
            auto opt = queue_.pop();
            if (!opt) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            tmp.swap(opt->vox);            // move
            try {
                db_.insert_bulk(tmp);
            } catch (const std::exception& e) {
                std::cerr << "[DB] " << e.what() << '\n';
            }
            tmp.clear();
        }
    }

    asio::io_context          io_;
    asio::ip::udp::socket     socket_;
    asio::ip::udp::endpoint   remote_;
    std::array<uint8_t, 2048> rx_buf_{};

    BatchQueue<>          queue_;
    db::PgPipeline&       db_;
    util::ThreadPool      pool_;
};
} // namespace sudp::net
