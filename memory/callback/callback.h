#pragma once

#include <memory>
#include <string>
#include <memory>

class OneTimeCallback {
public:
    virtual std::string operator()() const& = delete;
    virtual std::string operator()() const&& = 0;
    virtual ~OneTimeCallback() {
    }
};

class AwesomeCallback : public OneTimeCallback {
public:
    AwesomeCallback() = default;

    AwesomeCallback(AwesomeCallback&&) = default;

    AwesomeCallback(const std::string& message) : message_(new std::string(message)) {
    }

    std::string operator()() const&& override {
        std::string result = *message_;
        // delete this->message_;
        delete this;
        return result + "awesomeness";
    }

    ~AwesomeCallback() {
        if (message_ != nullptr) {
            delete message_;
        }
    }

private:
    std::string* message_ = nullptr;
};
