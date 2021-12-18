#pragma once

#include <unistd.h>
#include "sw_fwd.h"  // Forward declaration

#include <cstddef>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <memory>
#include <utility>
#include <iostream>

// https://en.cppreference.com/w/cpp/memory/shared_ptr


class ControlBlockBase {
public:
    size_t cnt_ = 0;
    size_t weak_cnt_ = 0;
    virtual ~ControlBlockBase() = default;

    virtual void Reset() = 0;
    virtual void Increase() = 0;
    virtual void Decrease() = 0;
};

template <typename T>
class ControlBlockWithPointer : public ControlBlockBase {
public:
    ControlBlockWithPointer(T* ptr) : ptr_(ptr) {
        cnt_ = 1;
    }
    ~ControlBlockWithPointer() {
        if (cnt_ == 0 && ptr_ != nullptr) {
            std::cerr << " DELETE " << std::endl;
            delete ptr_;
            ptr_ = nullptr;
        }
    }

    T* GetPtr() {
        return ptr_;
    }

    void Reset() override {
        if (cnt_ == 0) {
            return;
        }
        if (cnt_ == 1 && ptr_ != nullptr) {
            std::cerr << " DELETE " << std::endl;
            delete ptr_;
            ptr_ = nullptr;
        }
        --cnt_;
    }

    void Increase() override {
        ++cnt_;
    }
    virtual void Decrease() override {
        --cnt_;
    }

    T* ptr_;
};

template <typename T>
class ControlBlockWithElement : public ControlBlockBase {
public:
    template <typename... Args>
    ControlBlockWithElement(Args&&... args) {
        new (&aligned_value_) T(std::forward<Args>(args)...);
        ptr_ = reinterpret_cast<T*>(&aligned_value_);
        cnt_ = 1;
    }
    ~ControlBlockWithElement() {
        if (cnt_ == 0 && ptr_ != nullptr) {
            std::cerr << "Delete";
            ptr_->~T();
        }
    }

    void Reset() override {
        if (cnt_ == 0) {
            return;
        }
        if (cnt_ == 1 && ptr_ != nullptr) {
            std::cerr << "Delete";
            ptr_->~T();
            ptr_ = nullptr;
        }
        --cnt_;
        if (cnt_ == 0) {
            ptr_ = nullptr;
        }
    }

    void Increase() override {
        ++cnt_;
    }
    virtual void Decrease() override {
        --cnt_;
    }

    T* GetPtr() {
        return reinterpret_cast<T*>(&aligned_value_);
    }

    T* ptr_;
    typename std::aligned_storage<sizeof(T), alignof(T)>::type aligned_value_;
};

template <typename T>
class SharedPtr {
public:

    SharedPtr() {
        ptr_ = nullptr;
        control_block_ = nullptr;
    }
    SharedPtr(std::nullptr_t) {
        ptr_ = nullptr;
        control_block_ = nullptr;
    }
    explicit SharedPtr(T* ptr) {
        ptr_ = ptr;
        control_block_ = new ControlBlockWithPointer<T>(ptr);
        EnableHelper(ptr);
        std::cerr << "new block" << std::endl;
    }

    SharedPtr(T* ptr, ControlBlockBase* control_block) {
        control_block_ = control_block;
        ptr_ = ptr;
        EnableHelper(ptr);
    }

    template <typename F>
    explicit SharedPtr(F* ptr) {
        ptr_ = ptr;
        control_block_ = new ControlBlockWithPointer(ptr);
        EnableHelper(ptr);
        std::cerr << "new block" << std::endl;
    }

    SharedPtr(const SharedPtr& other) {
        ptr_ = other.Get();
        control_block_ = other.GetControlBlock();
        ControlBlockIncrease();
    }
    SharedPtr(SharedPtr&& other) {
        ptr_ = other.Get();
        control_block_ = other.GetControlBlock();
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
    }

    template <typename A>
    SharedPtr(const SharedPtr<A>& other) {
        ptr_ = other.Get();
        control_block_ = other.GetControlBlock();
        ControlBlockIncrease();
    }
    template <typename A>
    SharedPtr(SharedPtr<A>&& other) {
        ptr_ = other.Get();
        control_block_ = other.GetControlBlock();
        other.ptr_ = nullptr;
        other.control_block_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        ptr_ = ptr;
        control_block_ = other.GetControlBlock();
        ControlBlockIncrease();
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        ptr_ = other.ptr_;
        if (other.control_block_ == nullptr) {
            throw BadWeakPtr();
        } else {
            if (other.control_block_->cnt_ == 0) {
                throw BadWeakPtr();
            }
            control_block_ = other.control_block_;
            control_block_->cnt_++;
        }
    }


    SharedPtr& operator=(const SharedPtr& other) {
        if (&other == this) {
            return *this;
        }
        ControlBlockReset();
        DeleteControlBlock();
        ptr_ = other.Get();
        control_block_ = other.GetControlBlock();
        ControlBlockIncrease();  // ???
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        std::cerr << " !!!!!!!!! " << std::endl;
        if (&other == this) {
            return *this;
        }
        ControlBlockReset();
        DeleteControlBlock();
        ptr_ = other.Get();
        control_block_ = other.GetControlBlock();
        other.ptr_ = nullptr;
        other.control_block_ = nullptr;
        std::cerr << " !!!!!!!!! " << std::endl;
        return *this;
    }

    template <typename A>
    SharedPtr& operator=(const SharedPtr<A>& other) {
        ControlBlockReset();
        DeleteControlBlock();
        ptr_ = other.Get();
        control_block_ = other.GetControlBlock();
        ControlBlockIncrease();
        return *this;
    }

    template <typename A>
    SharedPtr& operator=(SharedPtr<A>&& other) {
        ControlBlockReset();
        DeleteControlBlock();
        ptr_ = other.Get();
        control_block_ = other.GetControlBlock();
        other.ptr_ = nullptr;
        other.control_block_ = nullptr;
        return *this;
    }


    ~SharedPtr() {
        if (control_block_) {
            ControlBlockReset();
        }
        DeleteControlBlock();
    }


    void Reset() { 
        ControlBlockReset();
        DeleteControlBlock();
        control_block_ = nullptr;
        ptr_ = nullptr;
    }
    void Reset(T* ptr) {
        ControlBlockReset();
        DeleteControlBlock();
        if (ptr == nullptr) {
            Reset();
        } else {
            control_block_ = new ControlBlockWithPointer<T>(ptr);
            std::cerr << "new block" << std::endl;
            ptr_ = ptr;
        }
    }
    template <typename A>
    void Reset(A* ptr) {  // ??????????????????
        ControlBlockReset();
        DeleteControlBlock();
        if (ptr == nullptr) {
            Reset();
        } else {
            control_block_ = new ControlBlockWithPointer<A>(ptr);
            std::cerr << "new block" << std::endl;
            ptr_ = ptr;
        }
    }
    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(control_block_, other.control_block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    }
    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }
    size_t UseCount() const {
        return (control_block_ ? control_block_->cnt_ : 0);
    }
    explicit operator bool() const {
        return ptr_ != nullptr;
    }

    ControlBlockBase* GetControlBlock() const {
        return control_block_;
    }

private:
    T* ptr_ = nullptr;
    ControlBlockBase* control_block_ = nullptr;

    void DeleteControlBlock() {
        if (control_block_ && control_block_->cnt_ + control_block_->weak_cnt_ == 0 &&
            control_block_ != nullptr) {
            delete control_block_;
            ptr_ = nullptr;
            std::cerr << " delete_block " << std::endl;
        }
    }

    void CreateNewControlBlock() {
        if (ptr_ != nullptr) {
            control_block_ = new ControlBlockWithPointer<T>(nullptr);
            std::cerr << "new block" << std::endl;
        }
    }

    void ControlBlockIncrease() {
        if (control_block_) {
            control_block_->Increase();
        }
    }

    void ControlBlockReset() {
        if (control_block_) {
            control_block_->Reset();
        }
    }

    template <typename A>
    std::enable_if_t<std::is_base_of_v<EnableSharedFromThis<A>, T>> EnableHelper(
        EnableSharedFromThis<A>* ptr) {
        std::cerr << " call first " << std::endl;
        ptr->weak_ = WeakPtr<T>(*this);
    }

    void EnableHelper(...) {
        std::cerr << " call second " << std::endl;
    }

    template <typename A>
    friend class SharedPtr;
    template <typename A>
    friend class WeakPtr;
    template <typename A, typename B>
    friend bool operator==(A, B);
    template <typename A>
    friend class EnableSharedFromThis;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return (left.Get() == right.Get()) && (left.GetControlBlock() == right.GetControlBlock());
}


template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    ControlBlockWithElement<T>* ctrl_block =
        new ControlBlockWithElement<T>(std::forward<Args>(args)...);
    T* ptr = ctrl_block->GetPtr();
    return SharedPtr<T>(ptr, ctrl_block);
}


template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(weak_);
    }
    SharedPtr<const T> SharedFromThis() const {
        return weak_;
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_;
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_;
    }

private:
    WeakPtr<T> weak_;
    template <typename A>
    friend class SharedPtr;
};
