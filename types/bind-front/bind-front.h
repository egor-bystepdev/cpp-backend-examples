#pragma once

#include <functional>
#include <tuple>
#include <utility>

template <class F, class... Args>
constexpr auto BindFront(F&& f, Args&&... args) {
    return [func = std::forward<F>(f),
            ... first_args = std::forward<Args>(args)](auto&&... back_args) mutable {
        return func(std::forward<decltype(first_args)>(first_args)...,
                    std::forward<decltype(back_args)>(back_args)...);
    };
}
