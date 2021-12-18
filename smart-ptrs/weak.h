#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"
#include <iostream>

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() {
    }

    WeakPtr(const WeakPtr& other) {
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        if (control_block_ != nullptr) {
            control_block_->weak_cnt_++;
        }
    }
    template <typename F>
    WeakPtr(const WeakPtr<F>& other) {
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        if (control_block_ != nullptr) {
            control_block_->weak_cnt_++;
        }
    }

    WeakPtr(WeakPtr&& other) {
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) {
        control_block_ = other.control_block_;
        ptr_ = other.ptr_;
        control_block_->weak_cnt_++;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this == &other) {
            return *this;
        }
        Reset();
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        if (control_block_ != nullptr) {
            control_block_->weak_cnt_++;
        }
        return *this;
    }
    template <typename U>
    WeakPtr& operator=(const WeakPtr<U>& other) {
        /*if (this == &other) {
            return *this;
        }*/
        Reset();
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        if (control_block_ != nullptr) {
            control_block_->weak_cnt_++;
        }
        return *this;
    }
    WeakPtr& operator=(WeakPtr&& other) {
        if (this == &other) {
            return *this;
        }
        Reset();
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        other.ptr_ = nullptr;
        other.control_block_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (control_block_ == nullptr) {
            return;
        }
        --control_block_->weak_cnt_;
        if (control_block_->cnt_ + control_block_->weak_cnt_ == 0) {
            delete control_block_;
        }
        control_block_ = nullptr;
        ptr_ = nullptr;
    }
    void Swap(WeakPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(control_block_, other.control_block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        UpdPtr();
        if (control_block_) {
            return ptr_ == nullptr ? 0 : control_block_->cnt_;
        }
    }
    bool Expired() const {
        if (control_block_ == nullptr) {
            return true;
        }
        if (control_block_->cnt_ == 0) {
            return true;
        }
        return false;
    }
    SharedPtr<T> Lock() const {
        if (control_block_ == nullptr) {
            return SharedPtr<T>(nullptr, new ControlBlockWithPointer<T>(nullptr));
        }
        UpdPtr();
        control_block_->cnt_++;
        return SharedPtr(ptr_, control_block_);
    }

private:
    void UpdPtr() const {
        if (control_block_ == nullptr) {
            ptr_ = nullptr;
        } else {
            if (control_block_->cnt_ == 0) {
                ptr_ = nullptr;
            }
        }
    }
    ControlBlockBase* control_block_ = nullptr;
    mutable T* ptr_ = nullptr;
    template <typename A>
    friend class SharedPtr;
    template <typename A>
    friend class WeakPtr;
};
