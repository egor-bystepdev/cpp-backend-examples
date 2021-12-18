#include "find_subsets.h"
#include <algorithm>
#include <random>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <ctime>

std::mutex g_i_mutex;

void Mitm(const std::vector<int64_t> data, uint64_t l, uint64_t r, size_t size_of_v,
          std::atomic<bool>& found, Subsets& result, std::unordered_map<int64_t, uint64_t>& subs,
          bool is_first_part, size_t shift, size_t bound) {
    for (uint64_t mask = l; mask < r; ++mask) {
        if (found) {
            return;
        }
        int64_t first = 0;
        int64_t second = 0;
        int64_t cnt_first = 0;
        int64_t cnt_second = 0;
        uint64_t e = mask;
        for (size_t i = 0; i < bound; ++i) {
            if (e % 3 == 1) {
                first += data[i + shift];
                cnt_first++;
            } else if (e % 3 == 2) {
                second += data[i + shift];
                cnt_second++;
            }
            e /= 3;
        }
        if (found) {
            return;
        }
        if (first == second && cnt_first && cnt_second) {
            const std::lock_guard<std::mutex> lock(g_i_mutex);
            if (found) {
                return;
            }

            found = true;
            uint64_t e = mask;

            for (size_t i = 0; i < bound; ++i) {
                if (e % 3 == 1) {
                    result.first_indices.push_back(shift + i);
                } else if (e % 3 == 2) {
                    result.second_indices.push_back(shift + i);
                }
                e /= 3;
            }
            result.exists = true;
            return;
        }
        if (is_first_part) {
            if (cnt_first || cnt_second) {
                const std::lock_guard<std::mutex> lock(g_i_mutex);
                subs[first - second] = mask;
            }
        } else {
            int64_t delta = second - first;
            if (subs.count(delta)) {
                int64_t cp_subs = subs[delta];
                int64_t e = cp_subs;
                for (size_t i = 0; i < shift; ++i) {
                    if (e % 3 == 1) {
                        cnt_first++;
                    } else if (e % 3 == 2) {
                        cnt_second++;
                    }
                    e /= 3;
                }
                if (cnt_first && cnt_second) {
                    const std::lock_guard<std::mutex> lock(g_i_mutex);
                    if (found) {
                        return;
                    }
                    found = true;
                    e = cp_subs;
                    for (size_t i = 0; i < shift; ++i) {
                        if (e % 3 == 1) {
                            result.first_indices.push_back(i);
                        } else if (e % 3 == 2) {
                            result.second_indices.push_back(i);
                        }
                        e /= 3;
                    }
                    e = mask;
                    for (size_t i = shift; i < size_of_v; ++i) {
                        if (e % 3 == 1) {
                            result.first_indices.push_back(i);
                        } else if (e % 3 == 2) {
                            result.second_indices.push_back(i);
                        }
                        e /= 3;
                    }
                    result.exists = true;
                    return;
                }
            }
        }
    }
}

size_t GetPow(size_t f) {
    size_t count_subset = 1;
    for (size_t i = 0; i < f; ++i) {
        count_subset *= 3;
    }
    return count_subset;
}

Subsets FindEqualSumSubsets(const std::vector<int64_t>& data) {
    size_t size_of_v = data.size();
    std::mt19937 mt_rand(std::time(nullptr));
    uint64_t count_subset = 1;
    size_t gh = static_cast<size_t>(5. * size_of_v / 11);
    for (size_t i = 0; i < gh; ++i) {
        count_subset *= 3;
    }
    Subsets result;
    result.exists = false;
    uint64_t threds_num = std::thread::hardware_concurrency();
    uint64_t step = (count_subset + threds_num - 1) / threds_num;
    std::vector<std::thread> workers;
    std::atomic<bool> found = false;
    std::unordered_map<int64_t, uint64_t> subs;
    subs.reserve(count_subset * 2);
    for (uint64_t i = 0; i < count_subset; i += step) {
        uint64_t l = i;
        uint64_t r = std::min(count_subset, i + step);
        workers.emplace_back(Mitm, data, l, r, size_of_v, std::ref(found), std::ref(result),
                             std::ref(subs), true, 0, gh);
    }
    for (auto& t : workers) {
        t.join();
    }
    workers.clear();
    if (result.exists) {
        return result;
    }
    count_subset = GetPow(size_of_v - gh);
    step = (count_subset + threds_num - 1) / threds_num;
    for (uint64_t i = 0; i < count_subset; i += step) {
        uint64_t l = i;
        uint64_t r = std::min(count_subset, i + step);
        workers.emplace_back(Mitm, data, l, r, size_of_v, std::ref(found), std::ref(result),
                             std::ref(subs), false, gh, size_of_v - gh);
    }
    for (auto& t : workers) {
        t.join();
    }
    return result;
}
