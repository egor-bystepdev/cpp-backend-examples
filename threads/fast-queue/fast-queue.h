#pragma once

#include <atomic>
#include <memory>
#include <queue>
#include <iostream>

template <class T>
class MPMCBoundedQueue {
public:
    struct Node {
        T value;
        std::atomic<int> generation;
    };

    explicit MPMCBoundedQueue(int size) {
        mod_ = size - 1;
        for (int i = 0; i < size; ++i) {
            v_.push_back(std::shared_ptr<Node>(new Node{T(), i}));
        }
    }

    bool Enqueue(const T& value) {
        while (42) {
            int index = end_.load();
            Node* vert = v_[index & mod_].get();
            int delta = vert->generation.load() - index;
            if (delta == 0) {
                if (end_.compare_exchange_weak(index, index + 1)) {
                    vert->value = value;
                    ++vert->generation;
                    return true;
                }
            } else if (delta == 1) {
                continue;
            } else {
                break;
            }
        }
        return false;
    }

    bool Dequeue(T& data) {
        while (42) {
            int index = start_.load();
            Node* vert = v_[index & mod_].get();
            int delta = vert->generation.load() - index;
            if (delta == 1) {
                if (start_.compare_exchange_weak(index, index + 1)) {
                    data = vert->value;
                    vert->generation += mod_;
                    return true;
                }
            } else if (delta > 1) {
                continue;
            } else {
                break;
            }
        }
        return false;
    }

private:
    std::vector<std::shared_ptr<Node>> v_;
    std::atomic<int> start_ = 0;
    std::atomic<int> end_ = 0;
    int mod_ = 0;
};
