#include <iostream>
#include <stdexcept>
#include <thread>

#include <asio.hpp>

#include "db/db_pool.hpp"
#include "db/spatial_pipeline.hpp"
#include "net/db_queue.hpp"
#include "net/session.hpp"         // ← ou udp_server.hpp si tu gardes l'ancienne version
#include "util/thread_pool.hpp"

int main() {
    try {
        // Connexion PostgreSQL (pool de connexions)
        sudp::db::DbPool db_pool("postgresql://user:password@db:5432/mydb", 4);

        // File partagée pour les données réassemblées (points ou blobs)
        sudp::net::DbQueue db_queue;

        // ThreadPool pour les workers de BD
        sudp::util::ThreadPool db_workers(2);  // ← ajustable

        // Lance les workers pour spatial_point
        for (int i = 0; i < 2; ++i) {
            db_workers.post([&db_queue, &db_pool] {
                std::cout << "[DB] Worker thread started\n";
                sudp::db::SpatialPipeline pipe(*db_pool.acquire());

                while (true) {
                    auto batch = db_queue.try_pop_batch();
                    if (!batch) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        continue;
                    }

                    for (auto& pt : batch->pts)
                        pipe.push(std::move(pt));
                }
            });
        }

        // Démarre le serveur UDP (Session = gestionnaire bas-niveau avec ASIO)
        asio::io_context io;
        asio::ip::udp::socket sock(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), 9000));
        sudp::net::Session session(std::move(sock), db_queue);

        std::cout << "[MAIN] Server is running on port 9000...\n";
        io.run();
    }
    catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << '\n';
        return 1;
    }
}