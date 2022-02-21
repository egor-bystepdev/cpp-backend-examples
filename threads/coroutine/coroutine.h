#pragma once

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
        throw std::runtime_error("nothinh to suspend");
    }
    current_coroutine->suspend();
}
