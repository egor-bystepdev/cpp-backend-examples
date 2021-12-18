#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <boost/context/continuation.hpp>
#include <stdexcept>
#include <iostream>

#include <functional>
#include <boost/context/continuation.hpp>
#include <stdexcept>
#include <iostream>

namespace ctx = boost::context;

class Coroutine;

thread_local Coroutine* current_coroutine = nullptr;

class Coroutine {
public:
    explicit Coroutine(std::function<void()> f) {
        c_ = ctx::callcc([&, f = std::move(f)](ctx::continuation&& sink) {
            susp_ = [&]() { sink = sink.resume(); };
            sink = sink.resume();
            f();
            return std::move(sink);
        });
        current_coroutine = this;
        flag_ = true;
        c_ = c_.resume();
    }

    void resume() {
        if (flag_) {
            throw std::runtime_error("in resume error");
        }
        if (!c_) {
            flag_ = false;
            throw std::runtime_error("context ended");
        }
        c_ = c_.resume();
    }

    void suspend() {
        flag_ = false;
        susp_();
    }

    ~Coroutine() {
        current_coroutine = nullptr;
    }

private:
    std::function<void()> susp_;
    boost::context::continuation c_;
    bool flag_ = false;
};

void suspend() {
    if (!current_coroutine) {
        std::cout << "whar" << std::endl;
        throw std::runtime_error("flex");
    }
    current_coroutine->suspend();
}

template <typename T>
class Yield {
public:
    template <typename U>
    void yield(U&& u) {
        elem = std::make_unique<T>(std::forward<U>(u));
        suspend();
        elem = nullptr;
    }
    std::unique_ptr<T> elem = nullptr;
};

template <typename T>
class Generator : public Yield<T> {
public:
    Generator(std::function<void(Yield<T>&)> f)
        : c_([&, f = std::move(f)]() {
              suspend();
              f(*this);
          }) {
    }
    const T* get() {
        try {
            c_.resume();
        } catch (std::runtime_error) {
            return nullptr;
        }
        return this->elem.get();
    }

private:
    Coroutine c_;
};
