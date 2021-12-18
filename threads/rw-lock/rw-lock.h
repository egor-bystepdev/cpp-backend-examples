#pragma once
#include <memory>
#include <mutex>
#include <iostream>
#include <thread>
#include <condition_variable>

class RWLock {
public:
    template <class Func>
    void Read(Func func) {
        global_.lock();
        if (write_) {
            global_.unlock();
            std::unique_lock<std::mutex> lock(waiting_mutex_);
            cv_.wait(lock, [&]() -> bool {
                global_.lock();
                if (write_) {
                    global_.unlock();
                    return false;
                }
                blocked_readers_++;
                global_.unlock();
                return true;
            });
        } else {
            blocked_readers_++;
            global_.unlock();
        }
        try {
            func();
        } catch (...) {
            EndRead();
            throw;
        }
        EndRead();
    }

    template <class Func>
    void Write(Func func) {
        global_.lock();
        if (write_ || blocked_readers_) {
            global_.unlock();
            std::unique_lock<std::mutex> lock(waiting_mutex_);
            cv_.wait(lock, [&]() -> bool {
                global_.lock();
                if (write_ || blocked_readers_) {
                    global_.unlock();
                    return false;
                }
                write_ = true;
                global_.unlock();
                return true;
            });
        } else {
            write_ = true;
            global_.unlock();
        }
        try {
            func();
        } catch (...) {
            EndWrite();
            throw;
        }
        EndWrite();
    }

private:
    std::mutex waiting_mutex_;
    std::mutex global_;
    int blocked_readers_ = 0;
    std::condition_variable cv_;
    bool write_ = false;

    void EndRead() {
        global_.lock();
        blocked_readers_--;
        if (!blocked_readers_) {
            cv_.notify_all();
        }
        global_.unlock();
    }

    void EndWrite() {
        global_.lock();
        write_ = false;
        cv_.notify_all();
        global_.unlock();
    }
};
