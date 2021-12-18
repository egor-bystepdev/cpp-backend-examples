#pragma once

#include <string>
#include <vector>
#include <memory>

struct State {
    int ref_count;
    std::vector<std::string> our_vector;

    State(const std::vector<std::string> other_vector) : ref_count(1), our_vector(other_vector) {
    }
};

class COWVector {
public:
    COWVector() {
        state_ = new State(std::vector<std::string>());
    }

    ~COWVector() {
        if (state_ != nullptr) {
            state_->ref_count--;
            if (state_->ref_count == 0) {
                delete state_;
                state_ = nullptr;
            }
        }
    }

    COWVector(const COWVector& other) : state_(other.state_) {
        state_->ref_count++;
    }

    COWVector& operator=(const COWVector& other) {
        state_->ref_count--;
        state_ = other.state_;
        state_->ref_count++;
        return *this;
    }

    size_t Size() const {
        return state_->our_vector.size();
    }

    void Resize(size_t size) {
        if (state_->ref_count > 1) {
            ResetState();
        }
        state_->our_vector.resize(size);
    }

    const std::string& Get(size_t at) {
        return state_->our_vector.data()[at];
    }

    const std::string& Back() {
        return state_->our_vector.back();
    }

    void PushBack(const std::string& value) {
        if (state_->ref_count > 1) {
            ResetState();
        }
        state_->our_vector.push_back(value);
    }

    void Set(size_t at, const std::string& value) {
        if (state_->ref_count > 1) {
            ResetState();
        }
        state_->our_vector[at] = value;
    }

private:
    void ResetState() {
        state_->ref_count--;
        state_ = new State(state_->our_vector);
    }

    State* state_;
};
