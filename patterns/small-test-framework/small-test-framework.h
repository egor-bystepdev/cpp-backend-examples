#pragma once

#include <memory>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <exception>
#include <vector>

class AbstractTest {
public:
    virtual void SetUp() = 0;
    virtual void TearDown() = 0;
    virtual void Run() = 0;
    virtual ~AbstractTest() {
    }
};

class TestFactory {
public:
    TestFactory() = default;
    template <typename F>
    TestFactory(F&& lambda) {
        lambda_test_ctor_ = std::move(lambda);
    }

    std::unique_ptr<AbstractTest> GetTest() const {
        if (!test_ptr_) {
            test_ptr_ = lambda_test_ctor_();
        }
        return std::move(test_ptr_);
    }

private:
    mutable std::unique_ptr<AbstractTest> test_ptr_;
    std::function<std::unique_ptr<AbstractTest>()> lambda_test_ctor_;
};

class TestRegistry {
public:
    TestRegistry(TestRegistry& other) = delete;
    TestRegistry(TestRegistry&& other) = delete;
    void operator=(TestRegistry&& other) = delete;

    static TestRegistry& Instance();

    template <class TestClass>
    void RegisterClass(const std::string& class_name) {
        test_set_.insert({class_name, TestFactory([]() {
                              return std::move(std::unique_ptr<TestClass>(new TestClass()));
                          })});
    }

    std::unique_ptr<AbstractTest> CreateTest(const std::string& class_name) {
        auto iter = test_set_.find({class_name, TestFactory()});
        if (iter == test_set_.end()) {
            throw std::out_of_range("");
        }
        return iter->second.GetTest();
    }

    void RunTest(const std::string& test_name) {
        std::unique_ptr<AbstractTest> ptr = CreateTest(test_name);
        ptr->SetUp();
        try {
            ptr->Run();
            ptr->TearDown();
        } catch (...) {
            ptr->TearDown();
            throw;
        }
    }

    template <class Predicate>
    std::vector<std::string> ShowTests(Predicate callback) const {
        std::vector<std::string> result;
        for (const auto& it : test_set_) {
            if (callback(it.first)) {
                result.push_back(it.first);
            }
        }
        return result;
    }

    std::vector<std::string> ShowAllTests() const {
        std::vector<std::string> result;
        for (const auto& it : test_set_) {
            result.push_back(it.first);
        }
        return result;
    }

    template <class Predicate>
    void RunTests(Predicate callback) {
        for (const auto& it : test_set_) {
            if (callback(it.first)) {
                RunTest(it.first);
            }
        }
    }

    void Clear() {
        test_set_.clear();
    }

private:
    struct Cmp {
        bool operator()(const std::pair<std::string, TestFactory>& first,
                        const std::pair<std::string, TestFactory>& second) const {
            return first.first < second.first;
        }
    };
    std::set<std::pair<std::string, TestFactory>, const Cmp> test_set_;
    TestRegistry() {
    }

protected:
    static std::unique_ptr<TestRegistry> singleton;
};

class FullMatch {
public:
    FullMatch(std::string& str) : str_(str) {
    }
    FullMatch(std::string&& str) : str_(std::move(str)) {
    }

    bool operator()(const std::string& other_str) {
        return other_str == str_;
    }

private:
    std::string str_;
};

class Substr {
public:
    Substr(std::string& str) : str_(str) {
    }

    Substr(std::string&& str) : str_(std::move(str)) {
    }
    bool operator()(const std::string& other_str) const {
        return other_str.find(str_) != std::string::npos;
    }

private:
    std::string str_;
};

std::unique_ptr<TestRegistry> TestRegistry::singleton = nullptr;
TestRegistry& TestRegistry::Instance() {
    if (!singleton) {
        singleton.reset(new TestRegistry());
    }
    return *singleton.get();
}
