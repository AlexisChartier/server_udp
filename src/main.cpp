#include <iostream>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#include <asio.hpp>
#include <libpq-fe.h>

#include "db/spatial_pipeline.hpp"
#include "net/session.hpp"

int main() {
    try {
        std::cout.setf(std::ios::unitbuf); // flush stdout after each output
        std::cerr.setf(std::ios::unitbuf); // flush stderr after each output
        std::cout << "[MAIN] Starting server...\n";

        // Carte : map de drones → pipeline DB
        std::unordered_map<std::string, std::unique_ptr<sudp::db::SpatialPipeline>> drone_pipelines;

        // Connexions PostgreSQL par drone (clé = ip:port en string)
        asio::io_context io;
        asio::ip::udp::socket sock(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), 9000));

        auto on_packet = [&](const std::string& drone_id, std::vector<sudp::db::PointRGB>&& points) {
            if (drone_pipelines.find(drone_id) == drone_pipelines.end()) {
                std::cout << "[MAIN] Creating pipeline for drone: " << drone_id << "\n";
                PGconn* conn = PQconnectdb("host=db port=5432 dbname=ros_db user=postgres password=pwd");
                if (!conn || PQstatus(conn) != CONNECTION_OK) {
                    std::cerr << "[DB] Connection failed for drone " << drone_id << ": " << PQerrorMessage(conn) << '\n';
                    return;
                }
                drone_pipelines[drone_id] = std::make_unique<sudp::db::SpatialPipeline>(conn);
            }

            for (auto& p : points) {
                drone_pipelines[drone_id]->push(std::move(p));
            }
            drone_pipelines[drone_id]->flush_pending();
        };

        // Lancement de la session UDP (async)
        std::unique_ptr<sudp::net::Session> session_ptr = std::make_unique<sudp::net::Session>(std::move(sock), on_packet);

        std::cout << "[MAIN] Server is running on port 9000...\n";
        io.run();
    }
    catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << '\n';
        return 1;
    }
}