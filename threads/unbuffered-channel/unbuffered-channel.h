#pragma once

#include <atomic>
#include <stdexcept>
#include <utility>
#include <optional>
#include <thread>
#include <condition_variable>
#include <queue>
#include <iostream>

template <typename T>
class UnbufferedChannel {
public:
    explicit UnbufferedChannel() {
        size_ = 1;
    }

    void Send(const T& value) {
        std::unique_lock<std::mutex> sending_lock(send_mutex_), lock(waiting_mutex_recv_);
        if (close_) {
            throw std::runtime_error("closed");
        }
        flag_send_ = true;
        flag_recv_ = false;
        vl_ = value;
        cv_recv_.notify_one();
        cv_send_.wait(lock, [&]() -> bool {
            if (!close_ && !flag_recv_) {
                return false;
            }
            if (!flag_recv_) {
                throw std::runtime_error("all recv are closed");
            }
            return true;
        });
    }

    std::optional<T> Recv() {
        std::optional<T> result = std::nullopt;
        std::unique_lock<std::mutex> recv_lock(recv_mutex_), lock(waiting_mutex_recv_);
        cv_recv_.wait(lock, [&]() -> bool {
            if (!close_ && !flag_send_) {
                return false;
            }
            if (close_) {
                return true;
            }
            flag_send_ = false;
            flag_recv_ = true;
            result = vl_;
            cv_send_.notify_one();
            return true;
        });
        return result;
    }

    void Close() {
        waiting_mutex_recv_.lock();
        close_ = true;
        cv_send_.notify_all();
        cv_recv_.notify_all();
        waiting_mutex_recv_.unlock();
    }

private:
    bool flag_send_ = false;
    bool flag_recv_ = false;
    T vl_ = T();
    size_t size_;
    std::mutex send_mutex_;
    std::mutex recv_mutex_;
    bool close_ = false;
    std::condition_variable cv_send_;
    std::condition_variable cv_recv_;
    std::mutex waiting_mutex_send_;
    std::mutex waiting_mutex_recv_;
};
