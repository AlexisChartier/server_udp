#include <iostream>
#include <stdexcept>
#include <thread>

#include <asio.hpp>

#include "db/db_pool.hpp"
#include "db/spatial_pipeline.hpp"
#include "net/db_queue.hpp"
#include "net/session.hpp"      
#include "util/thread_pool.hpp"

int main() {
    try {
        std::cout.setf(std::ios::unitbuf); // flush stdout after each output
        std::cerr.setf(std::ios::unitbuf); // flush stderr after each output
        std::cout << "[MAIN] Starting server...\n";
        std::cout << "[MAIN] Initializing...\n";
        std::cout << "[MAIN] Initializing database connection...\n";
        
        // Connexion PostgreSQL (pool de connexions)
        sudp::db::DbPool db_pool("postgresql://user:password@db:5432/ros_db", 4);

        // File partagée pour les données réassemblées (points ou blobs)
        sudp::net::DbQueue db_queue;

        // ThreadPool pour les workers de BD
        sudp::util::ThreadPool db_workers(2);  // ← ajustable

        // Lance les workers pour spatial_point
        for (int i = 0; i < 2; ++i) {
            db_workers.post([&db_queue, &db_pool] {
                auto guard = db_pool.acquire();               // 1. connexion protégée
                sudp::db::SpatialPipeline pipe(guard.get(),   // 2. même PGconn*
                                            1 /* batch size */);

                while (true) {
                    if (auto batch = db_queue.try_pop_batch(); batch) {
                        for (auto& pt : batch->pts)
                            pipe.push(std::move(pt));

                        pipe.flush_pending();                 // flush explicite
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    }
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