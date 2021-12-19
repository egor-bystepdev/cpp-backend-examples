#pragma once
#include <stdlib.h>
#include <glog/logging.h>

int16_t InvertWord(int16_t value, uint8_t cnt) {
    if (cnt == 0) {
        return 0;
    }
    if (((1 << (cnt - 1)) & value) != 0) {
        return value;
    } else {
        int16_t flex = (1 << cnt);
        value = value - flex + 1;
    }
    return value;
}

class UnZigZug {
public:
    UnZigZug() {
        int x_first = 0, y_first = 0;
        int x_second = 0, y_second = 1;
        for (size_t i = 0; i < 7; ++i) {
            if (i % 2) {
                int x = x_second, y = y_second;
                while (1) {
                    // //  //  DLOG(INFO) << x << " " << y;
                    q.push_back({x, y});
                    if (y == 0) {
                        break;
                    }
                    x++;
                    y--;
                }
                y_second += 2;
            } else {
                int x = x_first, y = y_first;
                while (1) {
                    q.push_back({x, y});
                    // //  //  DLOG(INFO) << x << " " << y;
                    if (x == 0) {
                        break;
                    }
                    x--;
                    y++;
                }
                x_first += 2;
            }
        }
        size_t e = q.size();
        for (size_t i = 0; i < 8; ++i) {
            q.push_back({i, 7 - i});
        }
        for (size_t i = e - 1; i < e; --i) {
            q.push_back({7 - q[i].first, 7 - q[i].second});
        }
        //  DLOG(INFO) << "check zigzag";
        std::vector<std::vector<int>> h(8, std::vector<int>(8));
        for (size_t i = 0; i < q.size(); ++i) {
            h[q[i].first][q[i].second] = i + 1;
        }
        for (size_t i = 0; i < 8; ++i) {
            std::string res;
            for (size_t j = 0; j < 8; ++j) {
                res += std::to_string(h[i][j]);
                res += " ";
            }
            //  DLOG(INFO) << res;
        }
    }

    void PrintMatrix(const std::vector<int16_t>& r) {
        std::vector<std::vector<int16_t>> h = GetVectorMatrix(r);
        std::string res = "\n";
        for (size_t i = 0; i < 8; ++i) {

            for (size_t j = 0; j < 8; ++j) {
                res += std::to_string(h[i][j]);
                res += " ";
            }
            res += "\n";
        }
        //  DLOG(INFO) << res;
    }

    std::vector<std::vector<int16_t>> GetVectorMatrix(const std::vector<int16_t>& r) {
        std::vector<std::vector<int16_t>> h(8, std::vector<int16_t>(8));
        for (size_t i = 0; i < r.size(); ++i) {
            h[q[i].first][q[i].second] = r[i];
        }
        return h;
    }

    std::vector<int16_t> GetVector(const std::vector<int16_t>& r) {
        std::vector<std::vector<int16_t>> h(8, std::vector<int16_t>(8));
        for (size_t i = 0; i < r.size(); ++i) {
            h[q[i].first][q[i].second] = r[i];
        }
        std::vector<int16_t> res;
        for (size_t i = 0; i < 8; ++i) {
            for (size_t j = 0; j < 8; ++j) {
                res.push_back(h[i][j]);
            }
        }
        return res;
    }

    std::vector<std::pair<int, int>> q;
};