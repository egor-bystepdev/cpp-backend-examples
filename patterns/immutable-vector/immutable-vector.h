#pragma once

#include <vector>
#include <memory>
#include <iostream>

template <class T>
class ImmutableVector {
private:
    template <typename Y>
    class BlockValue {
    public:
        BlockValue() {
            to_.resize(32);
        }

        BlockValue(const BlockValue& other) {
            value_ = other.value_;
            to_ = other.to_;
        }

        BlockValue(const T& value) : value_(value) {
            to_.resize(32);
        }

        Y& GetValue() {
            return value_;
        }

        void SetNext(size_t index, std::shared_ptr<BlockValue<Y>> f) {
            to_[index] = f;
        }

        std::shared_ptr<BlockValue<Y>> GetNext(size_t index) {
            return to_[index];
        }

    private:
        Y value_;
        std::vector<std::shared_ptr<BlockValue<Y>>> to_;
    };

    template <typename Y>
    class BorImpl {
    public:
        BorImpl() {
        }
        Y& ReturnValue(size_t index) const {
            std::shared_ptr<BlockValue<Y>> ptr = root_;
            while (1) {
                size_t last_bits = (index & 31);
                if (index == 0) {
                    return ptr.get()->GetValue();
                } else {
                    ptr = ptr.get()->GetNext(last_bits);
                }
                index >>= 5;
            }
        }

        std::shared_ptr<BlockValue<Y>> AddValue(size_t index, const Y& value) {
            std::shared_ptr<BlockValue<Y>> ptr(root_);
            if (!ptr.use_count()) {
                ptr.reset(new BlockValue<Y>(value));
            } else {
                ptr.reset(new BlockValue<Y>(*ptr.get()));
            }
            std::shared_ptr<BlockValue<Y>> copy_ptr(ptr);
            while (1) {
                size_t last_bits = (index & 31);
                if (index == 0) {
                    ptr.get()->GetValue() = value;
                    break;
                } else {
                    std::shared_ptr<BlockValue<Y>> nxt = ptr.get()->GetNext(last_bits);
                    if (nxt.use_count()) {
                        ptr.get()->SetNext(last_bits, std::shared_ptr<BlockValue<Y>>(
                                                          new BlockValue<Y>(*nxt.get())));
                    } else {
                        ptr.get()->SetNext(last_bits,
                                           std::shared_ptr<BlockValue<Y>>(new BlockValue<Y>()));
                    }
                    ptr = ptr.get()->GetNext(last_bits);
                }
                index >>= 5;
            }
            return copy_ptr;
        }

        BorImpl(std::shared_ptr<BlockValue<Y>> other) {
            root_ = other;
        }

        void ChangeRoot(std::shared_ptr<BlockValue<Y>> other) {
            root_ = other;
        }

        std::shared_ptr<BlockValue<Y>> GetRoot() {
            return root_;
        }

    private:
        std::shared_ptr<BlockValue<Y>> root_;

        template <typename A>
        friend class ImmutableVector;
    };

public:
    ImmutableVector() {
    }

    explicit ImmutableVector(size_t count, const T& value = T()) {
        for (size_t i = 0; i < count; ++i) {
            auto f = impl_.AddValue(i, value);
            impl_.root_ = f;
            size_ = i + 1;
        }
    }

    template <typename Iterator>
    ImmutableVector(Iterator first, Iterator last) {
        while (first != last) {
            auto f = impl_.AddValue(size_, *first);
            impl_.root_ = f;
            ++size_;
            first++;
        }
    }

    ImmutableVector(std::initializer_list<T> l) {
        for (const auto& it : l) {
            auto f = impl_.AddValue(size_, it);
            impl_.root_ = f;
            ++size_;
        }
    }

    ImmutableVector Set(size_t index, const T& value) {
        if (index >= size_) {
            return *this;
        }
        auto f = impl_.AddValue(index, value);
        ImmutableVector<T> result;
        result.impl_ = BorImpl<T>(f);
        result.size_ = size_;
        return result;
    }

    const T& Get(size_t index) const {
        static T pass_value = T();
        if (index >= size_) {
            return pass_value;
        }
        return impl_.ReturnValue(index);
    }

    ImmutableVector PushBack(const T& value) {
        auto f = impl_.AddValue(size_, value);
        ImmutableVector<T> result;
        result.impl_ = BorImpl<T>(f);
        result.size_ = size_ + 1;
        return result;
    }

    ImmutableVector PopBack() {
        if (!size_) {
            return *this;
        }
        ImmutableVector<T> result;
        result.impl_ = impl_;
        result.size_ = size_ - 1;
        return result;
    }

    size_t Size() const {
        return size_;
    }

private:
    BorImpl<T> impl_;
    size_t size_ = 0;
};
