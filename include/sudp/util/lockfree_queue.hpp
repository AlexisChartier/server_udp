#pragma once
#include <cstddef>
#include <optional>

#include <mutex>
#include <deque>
#define SUDP_HAS_BOOST_LOCKFREE 0


namespace sudp::util
{

template<typename T, std::size_t N = 8192>
class LockfreeQueue
{
public:
    bool push(const T& v) noexcept
    {
#if SUDP_HAS_BOOST_LOCKFREE
        return q_.push(v);
#else
        std::lock_guard<std::mutex> lk(m_);
        if (dq_.size() >= N) return false;
        dq_.push_back(v);
        return true;
#endif
    }

    std::optional<T> pop() noexcept
    {
#if SUDP_HAS_BOOST_LOCKFREE
        T out;
        if (q_.pop(out)) return out;
        return std::nullopt;
#else
        std::lock_guard<std::mutex> lk(m_);
        if (dq_.empty()) return std::nullopt;
        T v = std::move(dq_.front());
        dq_.pop_front();
        return v;
#endif
    }

private:
#if SUDP_HAS_BOOST_LOCKFREE
    boost::lockfree::queue<T, boost::lockfree::capacity<N>> q_;
#else
    std::mutex m_;
    std::deque<T> dq_;
#endif
};

} // namespace sudp::util
