#pragma once

#include <atomic>
#include <cstdint>

struct RWSpinLock {
public:
    void LockRead() {
        uint32_t expected = cnt_.load() & for_read_;
        while (!cnt_.compare_exchange_weak(expected, expected + 2)) {
            expected &= for_read_;
        }
    }

    void UnlockRead() {
        cnt_.fetch_sub(2);
    }

    void LockWrite() {
        uint32_t expected = 0;
        while (!cnt_.compare_exchange_weak(expected, 1)) {
            expected = 0;
        }
    }

    void UnlockWrite() {
        cnt_.exchange(0);
    }

private:
    const uint32_t for_read_ = UINT32_MAX - 1;
    std::atomic<uint32_t> cnt_ = 0;
};
