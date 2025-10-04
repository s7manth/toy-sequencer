#include "test_cases.cpp"
#include <iostream>
#include <memory>

int main() {
  std::cout << "=== Toy Sequencer Test Framework ===" << std::endl;
  std::cout << "Starting comprehensive test suite..." << std::endl;

  try {
    std::vector<std::unique_ptr<test_framework::TestSuite>> suites;

    // Add test suites
    suites.push_back(std::make_unique<test_framework::PingPongTestSuite>());
    suites.push_back(std::make_unique<test_framework::MarketDataTestSuite>());
    suites.push_back(std::make_unique<test_framework::FullSystemTestSuite>());

    // Run all test suites
    test_framework::TestRunner::run_multiple_suites(std::move(suites));

    std::cout << "\n=== All Tests Completed Successfully ===" << std::endl;
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Test framework error: " << e.what() << std::endl;
    return 1;
  }
}
