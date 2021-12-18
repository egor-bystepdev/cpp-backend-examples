#pragma once

#include <atomic>
#include <stdexcept>
#include <utility>
#include <optional>
#include <thread>
#include <condition_variable>
#include <queue>
#include <iostream>

template <class T>
class BufferedChannel {
public:
    explicit BufferedChannel(int size) {
        size_ = size;
    }

    void Send(const T& value) {
        std::unique_lock<std::mutex> lock(waiting_mutex_send_);
        auto predicate = [&]() -> bool {
            queue_mutex_.lock();
            if (close_) {
                queue_mutex_.unlock();
                throw std::runtime_error("kek");
            }
            if (q_.size() == size_) {
                queue_mutex_.unlock();
                return false;
            }
            q_.push(value);
            queue_mutex_.unlock();
            cv_recv_.notify_one();
            return true;
        };
        cv_send_.wait(lock, predicate);
    }

    std::optional<T> Recv() {
        std::optional<T> result = std::nullopt;
        std::unique_lock<std::mutex> lock(waiting_mutex_recv_);
        auto predicate = [&]() -> bool {
            queue_mutex_.lock();
            if (close_ && q_.empty()) {
                queue_mutex_.unlock();
                return true;
            }
            if (q_.empty()) {
                queue_mutex_.unlock();
                return false;
            }
            result = q_.front();
            q_.pop();
            queue_mutex_.unlock();
            cv_send_.notify_one();
            return true;
        };
        cv_recv_.wait(lock, predicate);
        return result;
    }

    void Close() {
        queue_mutex_.lock();
        close_ = true;
        queue_mutex_.unlock();
        cv_recv_.notify_all();
        cv_send_.notify_all();
    }

private:
    size_t size_;
    std::mutex queue_mutex_;
    bool close_ = false;
    std::condition_variable cv_send_;
    std::condition_variable cv_recv_;
    std::mutex waiting_mutex_send_;
    std::mutex waiting_mutex_recv_;
    std::queue<T> q_;
};
