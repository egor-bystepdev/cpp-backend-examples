#pragma once

#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include <mutex>
#include <atomic>

// Atomically do the following:
//    if (*value == expected_value) {
//        sleep_on_address(value)
//    }
void FutexWait(std::atomic<int> *value, int expected_value) {
    syscall(SYS_futex, value, FUTEX_WAIT_PRIVATE, expected_value, nullptr, nullptr, 0);
}

// Wakeup 'count' threads sleeping on address of value(-1 wakes all)
void FutexWake(std::atomic<int> *value, int count) {
    syscall(SYS_futex, value, FUTEX_WAKE_PRIVATE, count, nullptr, nullptr, 0);
}

int Cmpxchg(std::atomic<int> &value, int expected, int desired) {
    value.compare_exchange_strong(expected, desired);
    return expected;
}

class Mutex {
public:
    void Lock() {
        int c = Cmpxchg(cnt_, 0, 1);
        if (c != 0) {
            if (c != 2) {
                c = cnt_.exchange(2);
            }
            while (c != 0) {
                FutexWait(&cnt_, 2);
                c = cnt_.exchange(2);
            }
        }
    }

    void Unlock() {
        if (cnt_.fetch_sub(1) != 1) {
            cnt_ = 0;
            FutexWake(&cnt_, 1);
        }
    }

private:
    std::atomic<int> cnt_ = 0;
};
