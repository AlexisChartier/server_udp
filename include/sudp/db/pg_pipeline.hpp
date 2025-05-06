#pragma once
/**
 *  Insertion par lots dans la table `voxels`
 *  – décode Morton 10 × 3 → (x,y,z)
 *  – utilise COPY BINARY via libpq
 */
#include "db_pool.hpp"
#include "sudp/core/voxel.hpp"
#include "sudp/core/morton.hpp"
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
    void insert_bulk(const std::vector<common_dmw::Voxel>& voxels);
private:
    DbPool& pool_;
};
} // namespace sudp::db
