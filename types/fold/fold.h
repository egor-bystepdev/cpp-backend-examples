#pragma once

struct Sum {
    template <typename T>
    T operator()(T left, T right) {
        return left + right;
    }
};

struct Prod {
    template <typename T>
    T operator()(T left, T right) {
        return left * right;
    }
};

struct Concat {
    template <typename T>
    T operator()(T left, T right) {
        for (const auto& it : right) {
            left.push_back(it);
        }
        return left;
    }
};

template <class Iterator, class T, class BinaryOp>
T Fold(Iterator first, Iterator last, T init, BinaryOp func) {
    while (first != last) {
        init = func(init, *first);
        first++;
    }
    return init;
}

class Length {
private:
    int* cnt_ = nullptr;

public:
    Length() = delete;

    Length(int* cnt) : cnt_(cnt) {
    }

    template <typename T>
    T operator()(T left, T right) {
        *cnt_ = (*cnt_ + 1);
        return right;
    }
};
