#pragma once

#include <utility>
#include <tuple>

template <class F>
constexpr auto Curry(F&& f) {
    return [func = std::forward<F>(f)](auto&&... tp) mutable {
        return func(std::make_tuple(std::forward<decltype(tp)>(tp)...));
    };
}

template <class F>
constexpr auto Uncurry(F&& f) {
    return [func = std::forward<F>(f)](auto&& tp) mutable { return std::apply(func, tp); };
}
