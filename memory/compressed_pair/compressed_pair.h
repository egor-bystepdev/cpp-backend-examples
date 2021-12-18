#pragma once

#include <type_traits>
#include <memory>

template <class T, bool IsFirst, bool = std::is_empty_v<T> && !std::is_final_v<T>>
class CompressedPairElement {
public:
    CompressedPairElement() {
        element_ = T();
    }

    template <class S>
    CompressedPairElement(S&& other) : element_(std::forward<S>(other)) {
    }

    T& GetElement() {
        return element_;
    }

    const T& GetElement() const {
        return element_;
    }

private:
    T element_;
};

template <class T, bool IsFirst>
class CompressedPairElement<T, IsFirst, true> : T {
public:
    CompressedPairElement() = default;

    template <class S>
    CompressedPairElement(S&& other) : T(std::forward<S>(other)) {
    }

    T& GetElement() {
        return *this;
    }

    const T& GetElement() const {
        return *this;
    }
};

template <class F, class S>
class CompressedPair : CompressedPairElement<F, true>, CompressedPairElement<S, false> {
public:
    CompressedPair() = default;

    template <class Fe, class Se>
    CompressedPair(Fe&& first, Se&& second)
        : CompressedPairElement<F, true>(std::forward<Fe>(first)),
          CompressedPairElement<S, false>(std::forward<Se>(second)) {
    }

    F& GetFirst() {
        return CompressedPairElement<F, true>::GetElement();
    }

    S& GetSecond() {
        return CompressedPairElement<S, false>::GetElement();
    }

    const F& GetFirst() const {
        return CompressedPairElement<F, true>::GetElement();
    }

    const S& GetSecond() const {
        return CompressedPairElement<S, false>::GetElement();
    }
};
