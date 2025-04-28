#pragma once
/**
 *  Insertion par lots dans la table `voxels`
 *  – décode Morton 10 × 3 → (x,y,z)
 *  – utilise COPY BINARY via libpqxx::stream_to
 */
#include "db_pool.hpp"
#include "common_dmw/voxel.hpp"
#include "common_dmw/morton.hpp"
#include <vector>

namespace sudp::db
{
class PgPipeline
{
public:
    explicit PgPipeline(DbPool& pool) : pool_{pool} {}

    /**
     *  Insère un lot ; sans levée d’exception ⇒ succès.
     *  @throws pqxx::sql_error, std::runtime_error si erreur grave.
     */
    void insert_bulk(const std::vector<common_dmw::Voxel>& voxels)
    {
        auto conn_guard = pool_.acquire();
        pqxx::work tx(*conn_guard);

        pqxx::stream_to stream(
            tx, "COPY voxels (x,y,z,r,g,b) FROM STDIN BINARY");

        for (const auto& v : voxels) {
            uint32_t x,y,z;
            common_dmw::decode_morton10(v.morton, x, y, z);

            std::tuple<int,int,int,int16_t,int16_t,int16_t> row{
                static_cast<int>(x),
                static_cast<int>(y),
                static_cast<int>(z),
                static_cast<int16_t>((v.rgb >> 16) & 0xFF),
                static_cast<int16_t>((v.rgb >>  8) & 0xFF),
                static_cast<int16_t>( v.rgb        & 0xFF)
            };
            stream << row;
        }
        stream.complete();
        tx.commit();
    }

private:
    DbPool& pool_;
};
} // namespace sudp::db
