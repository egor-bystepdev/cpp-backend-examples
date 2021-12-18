#pragma once

#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <utility>
#include <array>
#include <iostream>
#include <cstddef>

template <class K, class V, int MaxSize = 8>
class ConstexprMap {
public:
    constexpr ConstexprMap() = default;

    constexpr V& operator[](const K& key) {
        for (size_t i = 0; i < size_; ++i) {
            if (map_[i].first == key) {
                return map_[i].second;
            }
        }
        if (size_ == MaxSize) {
            throw 1;
        }
        map_[size_] = std::make_pair(key, V());
        return map_[size_++].second;
    }

    constexpr const V& operator[](const K& key) const {
        if (Find(key)) {
            for (size_t i = 0; i < size_; ++i) {
                if (map_[i].first == key) {
                    return map_[i].second;
                }
            }
        } else {
            throw 2;
        }
    }

    constexpr bool Erase(const K& key) {
        for (size_t i = 0; i < size_; ++i) {
            if (map_[i].first == key) {
                for (size_t j = i; j + 1 < size_; ++j) {
                    map_[j] = map_[j + 1];
                }
                size_--;
                return true;
            }
            return false;
        }
    }

    constexpr bool Find(const K& key) const {
        for (size_t i = 0; i < size_; ++i) {
            if (map_[i].first == key) {
                return true;
            }
        }
        return false;
    }

    constexpr size_t Size() const {
        return size_;
    }

    constexpr std::pair<K, V>& GetByIndex(size_t pos) {
        return map_[pos];
    }

    constexpr const std::pair<K, V>& GetByIndex(size_t pos) const {
        return map_[pos];
    }

private:
    std::array<std::pair<K, V>, MaxSize> map_;
    size_t size_ = 0;
};
