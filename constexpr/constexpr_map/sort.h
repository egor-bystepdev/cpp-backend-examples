#pragma once

#include <constexpr_map.h>

#include <type_traits>

template <typename T>
constexpr void Swap(T& a, T& b) {
    T tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
}

template <class K, class V, int S>
constexpr auto Sort(ConstexprMap<K, V, S> map) {
    for (size_t i = 0; i < map.Size(); ++i) {
        for (size_t j = i + 1; j < map.Size(); ++j) {
            if constexpr (std::is_integral_v<K>) {
                if (map.GetByIndex(j).first > map.GetByIndex(i).first) {
                    Swap(map.GetByIndex(j).first, map.GetByIndex(i).first);
                    Swap(map.GetByIndex(j).second, map.GetByIndex(i).second);
                }
            } else {
                if (map.GetByIndex(j).first < map.GetByIndex(i).first) {
                    Swap(map.GetByIndex(j).first, map.GetByIndex(i).first);
                    Swap(map.GetByIndex(j).second, map.GetByIndex(i).second);
                }
            }
        }
    }
    return map;
}
