#pragma once

#include <stdlib.h>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <cassert>
#include <array>
#include <iostream>

struct Base {};

template <typename L, typename R>
struct Glue : Base {
    Glue(const L& l, const R& r) : left(l), right(r) {
    }

    const L& left;
    const R& right;
};

template <typename T>
struct GetCount {
    static const int kCount = 0;
};

template <class T>
class Matrix : Base {
public:
    Matrix(size_t n, size_t m) {
        if (n == 0 || m == 0) {
            throw std::invalid_argument("row or column eq zero");
        }
        matrix_.resize(n);
        std::fill(matrix_.begin(), matrix_.end(), std::vector<T>(m));
    }

    explicit Matrix(size_t n) : Matrix(n) {
    }

    Matrix(std::vector<std::vector<T>> m) {
        if (m.empty()) {
            throw std::invalid_argument("empty matrix");
        }
        auto iter = find_if(m.begin(), m.end(), [&](auto it) {
            if (it->size() != m.front().size()) {
                return true;
            }
            return false;
        });
        if (iter != m.end()) {
            throw std::invalid_argument("not matrix");
        }
        matrix_ = m;
    }

    Matrix(std::initializer_list<std::vector<T>> m) {
        if (m.size() == 0) {
            throw std::invalid_argument("empty matrix");
        }
        auto iter = find_if(m.begin(), m.end(), [&](auto it) {
            if (it.size() != m.begin()->size()) {
                return true;
            }
            return false;
        });
        if (iter != m.end()) {
            throw std::invalid_argument("not matrix");
        }
        matrix_ = m;
    }

    size_t Rows() const {
        return matrix_.size();
    }

    size_t Columns() const {
        return matrix_.front().size();
    }

    T& operator()(size_t i, size_t j) {
        if (i >= Rows() || j >= Columns()) {
            throw std::invalid_argument("op() invalid args");
        }
        return matrix_[i][j];
    }

    const T& operator()(size_t i, size_t j) const {
        if (i >= Rows() || j >= Columns()) {
            throw std::invalid_argument("op() invalid args");
        }
        return matrix_[i][j];
    }

    friend Matrix operator+(const Matrix& lhs, const Matrix& rhs) {
        if (lhs.Rows() != rhs.Rows() || lhs.Columns() != rhs.Columns()) {
            throw std::invalid_argument("sz");
        }
        Matrix result(rhs.Rows(), rhs.Columns());
        for (size_t i = 0; i < lhs.Rows(); ++i) {
            for (size_t j = 0; j < lhs.Columns(); ++j) {
                result(i, j) = lhs(i, j) + rhs(i, j);
            }
        }
        return result;
    }

    friend Matrix operator-(const Matrix& lhs, const Matrix& rhs) {
        if (lhs.Rows() != rhs.Rows() || lhs.Columns() != rhs.Columns()) {
            throw std::invalid_argument("sz");
        }
        Matrix result(rhs.Rows(), rhs.Columns());
        for (size_t i = 0; i < lhs.Rows(); ++i) {
            for (size_t j = 0; j < lhs.Columns(); ++j) {
                result(i, j) = lhs(i, j) - rhs(i, j);
            }
        }
        return result;
    }

    template <typename A, typename B>
    constexpr Matrix(const Glue<A, B>& rhs) {
        const int f = GetCount<Glue<A, B>>::kCount;
        std::array<const Matrix<T>*, f> mtrx;
        size_t s = GetRecursionProd(rhs, mtrx);
        for (size_t i = 1; i < s; ++i) {
            if (!CheckCorrectToProd(*mtrx[i - 1], *mtrx[i])) {
                throw std::runtime_error("sz");
            }
        }
        std::array<std::array<size_t, f>, f> dp, p;
        for (size_t ln = 0; ln < s; ++ln) {
            for (size_t i = 0; i + ln < s; ++i) {
                size_t j = i + ln;
                if (ln == 0) {
                    dp[i][j] = 0;
                    continue;
                }
                p[i][j] = i + 1;
                dp[i][j] =
                    mtrx[i]->Columns() * mtrx[i + 1]->Columns() * mtrx[i]->Rows() + dp[i + 1][j];
                for (size_t k = i + 1; k <= j; ++k) {
                    size_t t = dp[i][k - 1] + dp[k][j] +
                               mtrx[j]->Columns() * mtrx[k]->Rows() * mtrx[i]->Rows();
                    if (t < dp[i][j]) {
                        dp[i][j] = t;
                        p[i][j] = k;
                    }
                }
            }
        }
        Matrix g = DpResult(mtrx, p, 0, s - 1);
        matrix_ = g.matrix_;
    }

private:
    std::vector<std::vector<T>> matrix_;

    bool CheckCorrectToProd(const Matrix& lhs, const Matrix& rhs) {
        if (lhs.Columns() != rhs.Rows()) {
            return false;
        }
        return true;
    }

    template <typename F, size_t S>
    constexpr Matrix<F> DpResult(std::array<const Matrix<F>*, S>& mtrx,
                                 std::array<std::array<size_t, S>, S>& p, size_t i, size_t j) {
        if (i == j) {
            return *mtrx[i];
        }
        return MatrixesProd(DpResult(mtrx, p, i, p[i][j] - 1), DpResult(mtrx, p, p[i][j], j));
    }

    template <typename A, typename B, typename F, size_t S>
    constexpr size_t GetRecursionProd(const Glue<Matrix<A>, Matrix<B>>& obj,
                                      std::array<const Matrix<F>*, S>& mtrx) {
        mtrx[0] = &obj.left;
        mtrx[1] = &obj.right;
        return 2;
    }

    template <typename A, typename B, typename F, size_t S>
    constexpr size_t GetRecursionProd(const Glue<A, B>& obj,
                                      std::array<const Matrix<F>*, S>& mtrx) {
        size_t ind = GetRecursionProd(obj.left, mtrx);
        mtrx[ind] = &obj.right;
        return ind + 1;
    }

    template <typename F>
    constexpr Matrix<F> MatrixesProd(const Matrix<F>& lhs, const Matrix<F>& rhs) {
        if (lhs.Columns() != rhs.Rows()) {
            throw std::invalid_argument("sz");
        }
        Matrix<F> result(lhs.Rows(), rhs.Columns());
        for (size_t i = 0; i < lhs.Rows(); ++i) {
            for (size_t j = 0; j < rhs.Columns(); ++j) {
                for (size_t k = 0; k < rhs.Rows(); ++k) {
                    result(i, j) += lhs(i, k) * rhs(k, j);
                }
            }
        }
        return result;
    }
};

template <typename T>
struct GetCount<Matrix<T>> {
    static const int kCount = 1;
};

template <typename L, typename R>
struct GetCount<Glue<L, R>> {
    static const int kCount = GetCount<L>::kCount + GetCount<R>::kCount;
};

template <typename T>
Glue<Matrix<T>, Matrix<T>> operator*(const Matrix<T>& lhs, const Matrix<T>& rhs) {
    return Glue(lhs, rhs);
}

template <typename T>
concept IsAcceptedToMultiplyMatrixes = std::is_base_of_v<Base, T>;

template <IsAcceptedToMultiplyMatrixes A, IsAcceptedToMultiplyMatrixes B>
Glue<A, B> operator*(const A& lhs, const B& rhs) {
    return Glue(lhs, rhs);
}
