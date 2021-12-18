#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <memory>
#include <algorithm>
#include <utility>
#include <iostream>
#include <type_traits>
#include <memory>
#include <utility>
#include <type_traits>


template <typename T, typename Deleter = std::default_delete<T>>
class UniquePtr {
public:

    explicit UniquePtr(T* ptr = nullptr) : elem_(ptr, Deleter()) {
    }
    UniquePtr(T* ptr, const Deleter& deleter) : elem_(ptr, deleter) {
    }

    template <typename A, typename B>
    UniquePtr(A* ptr, B&& deleter) noexcept(std::is_move_constructible_v<B>)
        : elem_(ptr, std::forward<B>(deleter)) {
    }

    template <typename A, typename B>
    UniquePtr(UniquePtr<A, B>&& other) noexcept
        : elem_(other.Release(), std::forward<Deleter>(other.GetDeleter())) {
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (&other == this) {
            return *this;
        }
        Reset(other.Release());
        elem_.GetSecond() = std::forward<Deleter>(other.elem_.GetSecond());
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ~UniquePtr() {
        Reset();
    }

    T* Release() {
        T* element = elem_.GetFirst();
        elem_.GetFirst() = nullptr;
        return element;
    }
    void Reset(T* ptr = nullptr) {
        if (elem_.GetFirst() != ptr) {
            T* copy_ptr = elem_.GetFirst();
            elem_.GetFirst() = ptr;
            elem_.GetSecond()(copy_ptr);
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(elem_.GetFirst(), other.elem_.GetFirst());
        std::swap(elem_.GetSecond(), other.elem_.GetSecond());
    }

    T* Get() const {
        return elem_.GetFirst();
    }
    Deleter& GetDeleter() {
        return elem_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return elem_.GetSecond();
    }
    explicit operator bool() const {
        return elem_.GetFirst() != nullptr;
    }


    typename std::add_lvalue_reference<T>::type operator*() const {
        return *elem_.GetFirst();
    }
    T* operator->() const {
        return elem_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> elem_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    explicit UniquePtr(T* ptr = nullptr) : elem_(ptr, Deleter()) {
    }
    UniquePtr(T* ptr, const Deleter& deleter) : elem_(ptr, deleter) {
    }

    template <typename A, typename B>
    UniquePtr(A* ptr, B&& deleter) noexcept(std::is_move_constructible_v<B>)
        : elem_(ptr, std::forward<B>(deleter)) {
    }

    template <typename A, typename B>
    UniquePtr(UniquePtr<A, B>&& other) noexcept
        : elem_(other.Release(), std::forward<Deleter>(other.GetDeleter())) {
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (&other == this) {
            return *this;
        }
        Reset(other.Release());
        elem_.GetSecond() = std::forward<Deleter>(other.elem_.GetSecond());
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ~UniquePtr() {
        Reset();
    }

    T* Release() {
        T* element = elem_.GetFirst();
        elem_.GetFirst() = nullptr;
        return element;
    }
    void Reset(T* ptr = nullptr) {
        if (elem_.GetFirst() != ptr) {
            T* copy_ptr = elem_.GetFirst();
            elem_.GetFirst() = ptr;
            elem_.GetSecond()(copy_ptr);
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(elem_.GetFirst(), other.elem_.GetFirst());
        std::swap(elem_.GetSecond(), other.elem_.GetSecond());
    }

    T* Get() const {
        return elem_.GetFirst();
    }
    Deleter& GetDeleter() {
        return elem_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return elem_.GetSecond();
    }
    explicit operator bool() const {
        return elem_.GetFirst() != nullptr;
    }

    T& operator*() const {
        return *elem_.GetFirst();
    }
    T* operator->() const {
        return elem_.GetFirst();
    }

    T& operator[](int index) {
        return elem_.GetFirst()[index];
    }

    const T& operator[](int index) const {
        return elem_.GetFirst()[index];
    }

private:
    CompressedPair<T*, Deleter> elem_;
};
