#include "db/pg_pipeline.hpp"

namespace sudp::db

{

void PgPipeline::insert_bulk(const std::vector<common_dmw::Voxel>& voxels)

{

    auto conn_guard = pool_.acquire();

    pqxx::work tx(*conn_guard);

    pqxx::stream_to stream(

        tx, "COPY voxels (x,y,z,r,g,b) FROM STDIN BINARY");

    for (const auto& v : voxels) {

        uint32_t x, y, z;

        common_dmw::decode_morton10(v.morton, x, y, z);

        std::tuple<int, int, int, int16_t, int16_t, int16_t> row{

            static_cast<int>(x),

            static_cast<int>(y),

            static_cast<int>(z),

            static_cast<int16_t>((v.rgb >> 16) & 0xFF),

            static_cast<int16_t>((v.rgb >> 8) & 0xFF),

            static_cast<int16_t>(v.rgb & 0xFF)

        };

        stream << row;

    }

    stream.complete();

    tx.commit();

}

} 