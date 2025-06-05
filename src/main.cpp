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
                std::cout << "[DB] Worker thread started\n";
                sudp::db::SpatialPipeline pipe(*db_pool.acquire());

                while (true) {
                    auto batch = db_queue.try_pop_batch();
                    if (!batch) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        continue;
                    }
                    std::cout << "[DB Worker] Received batch with" << batch->pts.size() << " points \n";

                    for (auto& pt : batch->pts){
                        std::cout << "[DB Worker] Pushing point to pipeline\n";
                        std::cout << "[DB Worker] Point: " << pt.x << ", " << pt.y << ", " << pt.z << "\n";
                        pipe.push(std::move(pt));
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