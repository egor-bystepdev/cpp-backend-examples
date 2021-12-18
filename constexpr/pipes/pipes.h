#pragma once

#include <utility>
#include <memory>

template <typename F>
struct PipeLine {
    F func_;
    PipeLine(F functor) : func_(functor) {
    }
    constexpr auto operator|(auto f) const {
        auto lambda_f = [ansector = *this, f](auto&& arg) {
            return f(ansector.func_(std::move(arg)));
        };
        return PipeLine<decltype(lambda_f)>(lambda_f);
    }
    template <class Arg>
    constexpr auto operator()(Arg&& arg) const {
        return func_(std::move(arg));
    }
};

struct EmptyPlHelper {
    constexpr auto operator|(auto f) const {
        return PipeLine<decltype(f)>(f);
    }
};

constexpr auto kEmptyPl{EmptyPlHelper()};
