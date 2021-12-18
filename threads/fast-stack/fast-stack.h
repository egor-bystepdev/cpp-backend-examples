#pragma once

#include <atomic>
#include <optional>
#include <stdexcept>
#include <utility>

template <class T>
class MPSCStack {
public:
    struct Node {
        T value;
        Node* prev;
        Node(T x, Node* prev_ptr) : value(x), prev(prev_ptr) {
        }
    };

    // Safe to call from multiple threads.
    void Push(const T& value) {
        Node* new_head = new Node(value, head_);
        while (!head_.compare_exchange_weak(new_head->prev, new_head)) {
        }
    }

    // Not safe to call concurrently.
    std::optional<T> Pop() {
        Node* head_to_delete = head_;
        if (!head_to_delete) {
            return std::nullopt;
        }
        while (!head_.compare_exchange_weak(head_to_delete, head_to_delete->prev)) {
        }
        std::optional<T> result = head_to_delete->value;
        delete head_to_delete;
        return result;
    }

    // Not safe to call concurrently with Pop()
    template <class TFn>
    void DequeueAll(const TFn& cb) {
        std::optional<T> rs = std::nullopt;
        while ((rs = Pop()) != std::nullopt) {
            cb(rs.value());
        }
    }

    ~MPSCStack() {
        DequeueAll([](const auto& t) { return t; });
    }

private:
    std::atomic<Node*> head_ = nullptr;
};
