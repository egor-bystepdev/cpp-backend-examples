#pragma once

#include <atomic>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <thread>
#include <vector>
#include <list>
#include <utility>
#include <iostream>
#include <cmath>

template <class K, class V, class Hash = std::hash<K>>
class ConcurrentHashMap {
public:
    ConcurrentHashMap(const Hash& hasher = Hash()) : ConcurrentHashMap(kUndefinedSize, hasher) {
    }

    explicit ConcurrentHashMap(int expected_size, const Hash& hasher = Hash())
        : ConcurrentHashMap(expected_size, kDefaultConcurrencyLevel, hasher) {
    }

    ConcurrentHashMap(int expected_size, int expected_threads_count, const Hash& hasher = Hash())
        : hasher_(hasher) {
        (void)expected_threads_count;
        mutex_nums_ = expected_threads_count * 4;
        if (expected_size != kUndefinedSize) {
            expected_size = expected_size +
                            (expected_threads_count - expected_size % expected_threads_count) +
                            3 * expected_threads_count;
            table_.resize(expected_size);
            capacity_ = expected_size;
        } else {
            table_.resize(mutex_nums_);
            capacity_ = mutex_nums_;
        }
        for (size_t i = 0; i < mutex_nums_; ++i) {
            mutex_.push_back(std::unique_ptr<std::mutex>(new std::mutex()));
        }
    }

    bool Insert(const K& key, const V& value) {
        std::vector<std::unique_ptr<std::lock_guard<std::mutex>>> rs;
        GetCurrentPart(rs, key);
        size_t hashed_key = hasher_(key) % capacity_;
        for (const auto& it : table_[hashed_key]) {
            if (it.first == key) {
                return false;
            }
        }
        table_[hashed_key].emplace_back(std::make_pair(key, value));
        ++size_;
        rs.clear();
        if (size_ * expand_if_ > capacity_) {
            Expand();
        }
        return true;
    }

    bool Erase(const K& key) {
        std::vector<std::unique_ptr<std::lock_guard<std::mutex>>> rs;
        GetCurrentPart(rs, key);
        size_t hashed_key = hasher_(key) % capacity_;
        auto first = table_[hashed_key].begin(), last = table_[hashed_key].end();
        while (first != last) {
            if (first->first == key) {
                table_[hashed_key].erase(first);
                --size_;
                return true;
            }
            ++first;
        }
        return false;
    }

    void Clear() {
        std::vector<std::unique_ptr<std::lock_guard<std::mutex>>> rs;
        LockAll(rs);
        table_.clear();
        table_.resize(mutex_nums_);
        capacity_ = mutex_nums_;
        size_ = 0;
    }

    std::pair<bool, V> Find(const K& key) const {
        std::vector<std::unique_ptr<std::lock_guard<std::mutex>>> rs;
        GetCurrentPart(rs, key);
        size_t hashed_key = hasher_(key) % capacity_;
        auto first = table_[hashed_key].begin(), last = table_[hashed_key].end();
        while (first != last) {
            if (first->first == key) {
                return std::make_pair(true, first->second);
            }
            ++first;
        }
        return std::make_pair(false, V());
    }

    const V At(const K& key) const {
        std::vector<std::unique_ptr<std::lock_guard<std::mutex>>> rs;
        GetCurrentPart(rs, key);
        size_t hashed_key = hasher_(key) % capacity_;
        auto first = table_[hashed_key].begin(), last = table_[hashed_key].end();
        while (first != last) {
            if (first->first == key) {
                return first->second;
            }
            first++;
        }
        throw std::out_of_range("problems in At");
    }

    size_t Size() const {
        std::vector<std::unique_ptr<std::lock_guard<std::mutex>>> rs;
        LockAll(rs);
        return size_;
    }

    static const int kDefaultConcurrencyLevel;
    static const int kUndefinedSize;

private:
    Hash hasher_;
    size_t mutex_nums_;
    const size_t expand_size_ = 4;
    const double expand_if_ = 2;
    std::atomic<size_t> size_ = 0;
    std::atomic<size_t> capacity_ = 2;
    mutable std::mutex expand_mutex_;
    std::vector<std::list<std::pair<K, V>>> table_;
    mutable std::vector<std::unique_ptr<std::mutex>> mutex_;
    void LockAll(std::vector<std::unique_ptr<std::lock_guard<std::mutex>>>& rs) const {
        for (size_t i = 0; i < mutex_nums_; ++i) {
            rs.emplace_back(std::unique_ptr<std::lock_guard<std::mutex>>(
                new std::lock_guard<std::mutex>(*mutex_[i])));
        }
    }
    void GetCurrentPart(std::vector<std::unique_ptr<std::lock_guard<std::mutex>>>& rs,
                        const K& key) const {
        expand_mutex_.lock();
        size_t hashed_key = hasher_(key) % capacity_;
        size_t curr_ind = hashed_key / (capacity_ / mutex_nums_);
        rs.emplace_back(std::unique_ptr<std::lock_guard<std::mutex>>(
            new std::lock_guard<std::mutex>(*mutex_[curr_ind])));
        expand_mutex_.unlock();
    }
    void Expand() {

        expand_mutex_.lock();
        std::vector<std::unique_ptr<std::lock_guard<std::mutex>>> rs;
        LockAll(rs);
        expand_mutex_.unlock();
        if (size_ * expand_if_ < capacity_) {
            return;
        }
        size_t ls_cp = capacity_;
        capacity_ = capacity_ * expand_size_;
        table_.resize(capacity_);
        for (size_t i = 0; i < ls_cp; ++i) {
            if (table_[i].empty()) {
                continue;
            }
            auto it = table_[i].begin(), last = table_[i].end();
            last--;
            bool flag = false;
            while (true) {
                flag = (it == last);
                table_[hasher_(it->first) % capacity_].emplace_back(std::move(*it));
                table_[i].erase(it++);
                if (flag) {
                    break;
                }
            }
        }
    }
};

template <class K, class V, class Hash>
const int ConcurrentHashMap<K, V, Hash>::kDefaultConcurrencyLevel = 8;

template <class K, class V, class Hash>
const int ConcurrentHashMap<K, V, Hash>::kUndefinedSize = -1;
