#include "net/udp_server.hpp"

namespace sudp::net

{

UdpServer::UdpServer(uint16_t port, db::PgPipeline& db, std::size_t n_workers)

    : socket_{io_, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)}

    , db_{db}

    , pool_{n_workers}

{

    start_receive();

    pool_.post([this]{ db_loop(); });

}

void UdpServer::run()

{

    io_.run();

}

void UdpServer::start_receive()

{

    socket_.async_receive_from(

        asio::buffer(rx_buf_), remote_,

        [this](auto ec, std::size_t len)

        {

            if (!ec && len > 0) {

                pool_.post([this, len] {

                    Session{rx_buf_.data(), len, queue_};

                });

            }

            start_receive();

        });

}

void UdpServer::db_loop()

{

    std::vector<common_dmw::Voxel> tmp;

    tmp.reserve(2048);

    while (true)

    {

        auto opt = queue_.pop();

        if (!opt) {

            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            continue;

        }

        tmp.swap(opt->vox);

        try {

            db_.insert_bulk(tmp);

        } catch (const std::exception& e) {

            std::cerr << "[DB] " << e.what() << '\n';

        }

        tmp.clear();

    }

}

} // namespace sudp::net