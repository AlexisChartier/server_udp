#include "util/thread_pool.hpp"

namespace sudp::util

{

ThreadPool::ThreadPool(std::size_t n)

{

    for (std::size_t i = 0; i < n; ++i)

    {

        workers_.emplace_back([this] { worker_loop(); });

    }

}

ThreadPool::~ThreadPool()

{

    stop_ = true;

    cv_.notify_all();

    for (auto& t : workers_)

    {

        if (t.joinable())

            t.join();

    }

}

void ThreadPool::worker_loop()

{

    while (!stop_)

    {

        std::function<void()> job;

        {

            std::unique_lock<std::mutex> lk(m_);

            cv_.wait(lk, [this] { return stop_ || !q_.empty(); });

            if (stop_ && q_.empty())

                return;

            job = std::move(q_.front());

            q_.pop();

        }

        job();

    }

}

} // namespace sudp::util
