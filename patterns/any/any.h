#pragma once
#include <memory>
#include <algorithm>
#include <iostream>

class Any {
private:
    class StorageBase {
    public:
        virtual ~StorageBase() = default;
    };

    template <typename T>
    class Storage : public StorageBase {
    public:
        Storage(const T& value) : value_(value) {
        }

        const T& GetValue() const {
            return value_;
        }

    private:
        T value_;
    };

public:
    Any() : ptr_(nullptr) {
    }

    template <class T>
    Any(const T& value) : ptr_(new Storage<T>(value)) {
    }

    template <class T>
    Any& operator=(const T& value) {
        ptr_.reset(new Storage<T>(value));
        return *this;
    }

    Any(const Any& rhs) : ptr_(rhs.ptr_) {
    }

    Any& operator=(const Any& rhs) {
        Any copy_value(rhs);
        Swap(copy_value);
        return *this;
    }

    ~Any() = default;

    bool Empty() const {
        return !ptr_.use_count();
    }

    void Clear() {
        ptr_.reset();
    }
    void Swap(Any& rhs) {
        std::swap(ptr_, rhs.ptr_);
    }

    template <class T>
    const T& GetValue() const {
        auto ptr = std::dynamic_pointer_cast<Storage<T>>(ptr_);
        if (!ptr) {
            throw std::bad_cast();
        }
        return ptr->GetValue();
    }

private:
    //  std::unique_ptr<StorageBase> ptr_;
    std::shared_ptr<StorageBase> ptr_;
};
