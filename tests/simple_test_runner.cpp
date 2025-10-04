#include "simple_test_example.cpp"
#include <csignal>
#include <iostream>
#include <memory>

namespace {
std::unique_ptr<test_framework::TestSuite> g_test_suite = nullptr;

void signal_handler(int signal) {
  std::cout << "\nReceived signal " << signal << ", cleaning up..." << std::endl;
  if (g_test_suite) {
    g_test_suite->ensure_cleanup();
  }
  exit(1);
}
} // namespace

int main() {
  std::cout << "=== Simple Toy Sequencer Test Example ===" << std::endl;
  std::cout << "Running basic ping-pong test..." << std::endl;

  // Set up signal handlers for clean shutdown
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  try {
    g_test_suite = std::make_unique<test_framework::SimplePingPongTestSuite>();
    test_framework::TestRunner::run_suite(std::move(g_test_suite));

    std::cout << "\n=== Test Completed Successfully ===" << std::endl;
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Test error: " << e.what() << std::endl;
    return 1;
  }
}
