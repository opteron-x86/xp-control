#pragma once

#include <atomic>
#include <cstdint>

namespace xp_control {
enum class Phase { menu, loading, playing };

// Each queued task belongs to a session epoch. Revert invalidates queued work.
class Session {
public:
    std::uint64_t Reset(Phase phase) {
        phase_.store(phase);
        return ++epoch_;
    }
    [[nodiscard]] std::uint64_t Epoch() const { return epoch_.load(); }
    [[nodiscard]] Phase Current() const { return phase_.load(); }
    [[nodiscard]] bool Accepts(std::uint64_t epoch) const {
        return epoch == epoch_.load() && phase_.load() != Phase::loading;
    }
private:
    std::atomic<std::uint64_t> epoch_{1};
    std::atomic<Phase> phase_{Phase::menu};
};

class QueuedWork {
public:
    bool Begin(std::uint64_t epoch) {
        std::uint64_t empty = 0;
        return owner_.compare_exchange_strong(empty, epoch);
    }
    void Complete(std::uint64_t epoch) { owner_.compare_exchange_strong(epoch, 0); }
    void Cancel() { owner_.store(0); }
    [[nodiscard]] bool Busy() const { return owner_.load() != 0; }
private:
    std::atomic<std::uint64_t> owner_{0};
};
}
