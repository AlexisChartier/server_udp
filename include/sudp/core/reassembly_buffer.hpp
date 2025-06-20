#pragma once

#include <vector>
#include <cstddef>
#include <cstring>
#include <cstdint>

namespace sudp::core {

/** Buffer de ré‑assemblage (UDP fragment → blob). */
class ReassemblyBuffer {
public:
    explicit ReassemblyBuffer(std::size_t total)
        : total_(total), received_(0), data_(total, 0) {}

    bool write(std::size_t off, const uint8_t* src, std::size_t len) {
        if (off + len > total_) return false;
        std::memcpy(data_.data() + off, src, len);
        received_ += len;
        return true;
    }
    [[nodiscard]] bool complete() const noexcept { return received_ >= total_; }
    [[nodiscard]] auto& data()       noexcept { return data_; }
    std::size_t total() const noexcept { return total_; }

private:
    std::size_t          total_;
    std::size_t          received_;
    std::vector<uint8_t> data_;
};

} // namespace sudp::core

