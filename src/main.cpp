#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "db/db_pool.hpp"
#include "db/pg_pipeline.hpp"
#include "net/udp_server.hpp"


int main()

{

    try

    {

        // Crée le pool de connexions PostgreSQL (par ex: 4 connexions)

        sudp::db::DbPool db_pool("postgresql://user:password@db:5432/mydb", 4);

        // Prépare le pipeline d'insertion

        sudp::db::PgPipeline pipeline(db_pool);

        // Lance le serveur UDP

        sudp::net::UdpServer server(9000, pipeline, std::thread::hardware_concurrency());

        // Exécute le serveur (boucle infinie)
        std::cout << "Server is running on port 9000...\n";
        server.run();
    }
    catch (const std::exception& e)

    {
        std::cerr << "[Fatal] " << e.what() << '\n';
        return 1;

    }

}