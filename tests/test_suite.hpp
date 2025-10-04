#pragma once

#include "test_harness.hpp"
#include <string>
#include <utility>
#include <vector>

namespace test_framework {

class TestSuite {
public:
  TestSuite(std::string name) : name_(std::move(name)) {}
  virtual ~TestSuite() {
    ensure_cleanup();
  }

  virtual void setup() {}
  virtual void teardown() {}
  virtual void run_tests() = 0;

  void ensure_cleanup() {
    try {
      harness_.get_event_collector().stop();
      harness_.stop_all();
    } catch (const std::exception &e) {
      std::cerr << "Cleanup error: " << e.what() << std::endl;
    }
  }

  const std::string &get_name() const { return name_; }

  void add_test(const std::string &test_name, std::function<void()> test_func) {
    tests_.emplace_back(test_name, test_func);
  }

  void run_all_tests() {
    std::cout << "\n=== Running Test Suite: " << name_ << " ===" << std::endl;

    try {
      setup();

      int passed = 0;
      int failed = 0;

      for (const auto &[test_name, test_func] : tests_) {
        std::cout << "\n--- Running Test: " << test_name << " ---" << std::endl;
        try {
          test_func();
          std::cout << "✓ PASSED: " << test_name << std::endl;
          passed++;
        } catch (const std::exception &e) {
          std::cout << "✗ FAILED: " << test_name << " - " << e.what() << std::endl;
          failed++;
        }
      }

      teardown();

      std::cout << "\n=== Test Suite Results ===" << std::endl;
      std::cout << "Total: " << (passed + failed) << ", Passed: " << passed
                << ", Failed: " << failed << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "Test suite error: " << e.what() << std::endl;
      ensure_cleanup();
      throw;
    }
  }

protected:
  TestHarness harness_;

private:
  std::string name_;
  std::vector<std::pair<std::string, std::function<void()>>> tests_;
};

class TestRunner {
public:
  static void run_suite(std::unique_ptr<TestSuite> suite) {
    suite->run_tests();
    suite->run_all_tests();
  }

  static void run_multiple_suites(std::vector<std::unique_ptr<TestSuite>> suites) {
    for (auto &suite : suites) {
      suite->run_tests();
      suite->run_all_tests();
    }

    std::cout << "\n=== Overall Test Results ===" << std::endl;
    std::cout << "All test suites completed." << std::endl;
  }
};

}
