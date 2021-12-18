#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

template <class F, class... Args>
using TransformResult = decltype(std::tuple<std::invoke_result_t<F, Args>...>());  /// 1

template <class F, class... Types>
concept invocable_on_all_types = (std::is_invocable<F, Types>::value && ...);  /// 2

template <class F, size_t Index = 0, class... Args>
constexpr auto TransformResultImpl(F&& functor, const std::tuple<Args...>& args) {
    if constexpr (Index == sizeof...(Args)) {
        return std::tuple();
    } else {
        return std::tuple_cat(std::tuple(functor(std::get<Index>(args))),
                              TransformResultImpl<F, Index + 1, Args...>(std::move(functor), args));
    }
}

template <class F, size_t Index = 0, class... Args>
constexpr TransformResult<F, Args...> TransformTuple(
    F&& functor, const std::tuple<Args...>& args) requires invocable_on_all_types<F, Args...> {
    return TransformResultImpl(std::move(functor), args);  /// 3
}

template <class F, size_t Index = 0, class... Args>
constexpr auto TransformReduceTupleImpl(F&& functor, const std::tuple<Args...>& args) {
    if constexpr (Index == sizeof...(Args)) {
        return std::tuple();
    } else if constexpr (std::is_void<decltype(functor(std::get<Index>(args)))>::value) {
        return TransformReduceTupleImpl<F, Index + 1, Args...>(std::move(functor), args);
    } else {
        return std::tuple_cat(
            std::tuple(functor(std::get<Index>(args))),
            TransformReduceTupleImpl<F, Index + 1, Args...>(std::move(functor), args));
    }
}

template <class F, size_t Index = 0, class... Args>
constexpr auto TransformReduceTuple(
    F&& functor, const std::tuple<Args...>& args) requires invocable_on_all_types<F, Args...> {
    return TransformReduceTupleImpl(std::move(functor), args);
}
