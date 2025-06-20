#pragma once
#include <unordered_map>
#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>
#include <iostream>
#include <libpq-fe.h>
#include <deque>

#include "db/spatial_pipeline.hpp"

namespace sudp::net {

class DroneManager {
public:
    static DroneManager& instance() {
        static DroneManager inst;
        return inst;
    }

    void handle(const std::string& drone_id, std::vector<db::PointRGB>&& pts) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (workers_.find(drone_id) == workers_.end()) {
            std::cout << "[DroneManager] New drone detected: " << drone_id << "\n";
            workers_[drone_id] = std::thread([this, drone_id]() {
                thread_loop(drone_id);
            });
        }

        queues_[drone_id].emplace_back(std::move(pts));
    }

    ~DroneManager() {
        for (auto& [_, t] : workers_) {
            if (t.joinable()) t.join();
        }
    }

private:
    DroneManager() {
        conn_ = PQconnectdb("postgresql://user:password@db:5432/ros_db");
        if (PQstatus(conn_) != CONNECTION_OK) {
            throw std::runtime_error("[DroneManager] Failed to connect to PostgreSQL");
        }
    }

    void thread_loop(const std::string& drone_id) {
        db::SpatialPipeline pipe(conn_, 1000);

        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            std::vector<db::PointRGB> batch;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!queues_[drone_id].empty()) {
                    batch = std::move(queues_[drone_id].front());
                    queues_[drone_id].pop_front();
                }
            }

            for (auto& pt : batch) {
                pipe.push(std::move(pt));
            }
        }
    }

    PGconn* conn_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::thread> workers_;
    std::unordered_map<std::string, std::deque<std::vector<db::PointRGB>>> queues_;
};

} // namespace sudp::net