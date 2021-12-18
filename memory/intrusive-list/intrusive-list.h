#pragma once

#include <algorithm>
#include <cstddef>

class ListHook {
public:
    ListHook() {
    }

    bool IsLinked() const {
        return prev_ && next_;
    }

    void Unlink() {
        if (prev_) {
            prev_->next_ = next_;
        }
        if (next_) {
            next_->prev_ = prev_;
        }
        prev_ = next_ = nullptr;
    }

    ~ListHook() {
        Unlink();
    }

    ListHook(const ListHook&) = delete;

private:
    template <class T>
    friend class List;
    ListHook* prev_ = nullptr;
    ListHook* next_ = nullptr;
    void LinkBefore(ListHook* other) {
        if (other->prev_ == nullptr) {
            other->prev_ = this;
            next_ = other;
        } else {
            //
            prev_ = other->prev_;
            other->prev_ = this;
            next_ = other;
            prev_->next_ = this;
        }
    }
};

template <typename T>
class List {
public:
    class Iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
    public:
        Iterator(ListHook* element) {
            element_ = element;
        }

        Iterator& operator++() {
            element_ = element_->next_;
            return *this;
        }
        Iterator operator++(int) {
            Iterator* last = this;
            element_ = element_->next_;
            return *last;
        }

        T& operator*() const {
            return *static_cast<T*>(element_);
        }
        T* operator->() const {
            return static_cast<T*>(element_);
        }

        bool operator==(const Iterator& rhs) const {
            return rhs.element_ == element_;
        }
        bool operator!=(const Iterator& rhs) const {
            return !(rhs == *this);
        }

    private:
        ListHook* element_ = nullptr;
    };

    List() {
        begin_.LinkBefore(&end_);
    }
    List(const List&) = delete;
    List(List&& other) {
        begin_.next_ = other.begin_.next_;
        end_.prev_ = other.end_.prev_;
        other.begin_ = ListHook();
        other.end_ = ListHook();
        other.begin_.LinkBefore(&other.end_);
    }

    ~List() {
        while (!IsEmpty()) {
            end_.prev_->Unlink();
        }
    }

    List& operator=(const List&) = delete;
    List& operator=(List&& other) {
        if (this == &other) {
            return *this;
        }
        this->~List();
        new (this) List(std::move(other));
        return *this;
    }

    bool IsEmpty() const {
        return begin_.next_ == &end_;
    }
    size_t Size() const {
        ListHook* it = begin_.next_;
        size_t size = 0;
        while (it != &end_) {
            it = it->next_;
            ++size;
        }
        return size;
    }

    void PushBack(T* elem) {
        static_cast<ListHook*>(elem)->LinkBefore(&end_);
    }
    void PushFront(T* elem) {
        static_cast<ListHook*>(elem)->LinkBefore(begin_.next_);
    }

    T& Front() {
        return *static_cast<T*>(begin_.next_);
    }
    const T& Front() const {
        return *static_cast<T*>(begin_.next_);
    }

    T& Back() {
        return *static_cast<T*>(end_.prev_);
    }
    const T& Back() const {
        return *static_cast<T*>(end_.prev_);
    }

    void PopBack() {
        end_.prev_->Unlink();
    }
    void PopFront() {
        begin_.next_->Unlink();
    }

    Iterator Begin() {
        return Iterator(begin_.next_);
    }
    Iterator End() {
        return Iterator(&end_);
    }

    Iterator IteratorTo(T* element) {
        return Iterator(static_cast<ListHook*>(element));
    }

private:
    ListHook begin_ = ListHook();
    ListHook end_ = ListHook();
};

template <typename T>
typename List<T>::Iterator begin(List<T>& list) {
    return list.Begin();
}

template <typename T>
typename List<T>::Iterator end(List<T>& list) { 
    return list.End();
}
