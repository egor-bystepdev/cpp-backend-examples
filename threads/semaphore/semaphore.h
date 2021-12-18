#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

class DefaultCallback {
public:
    void operator()(int& value) {
        --value;
    }
};

class Semaphore {
public:
    Semaphore(int count) : count_(count) {
    }

    void Leave() {
        std::unique_lock<std::mutex> lock(mutex_);
        ++count_;
        cv_.notify_all();
    }

    template <class Func>
    void Enter(Func callback) {
        muteq_q_.lock();
        numbers_in_q_.push(std::this_thread::get_id());
        muteq_q_.unlock();
        std::unique_lock<std::mutex> lock(mutex_);
        while (!Predicate()) {
            cv_.wait(lock);
        }
        callback(count_);
    }

    void Enter() {
        DefaultCallback callback;
        Enter(callback);
    }

private:
    std::mutex mutex_;
    std::mutex muteq_q_;
    std::queue<std::thread::id> numbers_in_q_;
    std::condition_variable cv_;
    bool Predicate() {
        std::lock_guard<std::mutex> lk(muteq_q_);
        if (count_ && !numbers_in_q_.empty() &&
            numbers_in_q_.front() == std::this_thread::get_id()) {
            numbers_in_q_.pop();
            return true;
        }
        return false;
    }

    int count_ = 0;
};
