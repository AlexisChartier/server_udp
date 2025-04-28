#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <functional>

namespace sudp::util
{
class ThreadPool
{
public:
    explicit ThreadPool(std::size_t n)
    {
        for (std::size_t i=0;i<n;++i)
            workers_.emplace_back([this]{ worker_loop(); });
    }
    ~ThreadPool()
    {
        stop_ = true;
        cv_.notify_all();
        for (auto& t : workers_) if (t.joinable()) t.join();
    }

    template<typename F>
    void post(F&& f)
    {
        {
            std::lock_guard<std::mutex> lk(m_);
            q_.emplace(std::forward<F>(f));
        }
        cv_.notify_one();
    }

private:
    void worker_loop()
    {
        while (!stop_)
        {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lk(m_);
                cv_.wait(lk, [&]{return stop_ || !q_.empty();});
                if (stop_ && q_.empty()) return;
                job = std::move(q_.front());
                q_.pop();
            }
            job();
        }
    }

    std::vector<std::thread> workers_;
    std::mutex              m_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> q_;
    std::atomic<bool>       stop_{false};
};
} // namespace sudp::util
