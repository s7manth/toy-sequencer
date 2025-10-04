#include "test_suite.hpp"
#include <iostream>
#include <thread>

namespace test_framework {

class SimplePingPongTestSuite : public TestSuite {
public:
  SimplePingPongTestSuite() : TestSuite("Simple PingPong Tests") {}

  void setup() override {
    std::cout << "Setting up simple test..." << std::endl;

    // Start only sequencer for now (ping/pong have port conflicts)
    assert_true(harness_.start_sequencer(), "Failed to start sequencer");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::cout << "Simple test environment ready" << std::endl;
  }

  void teardown() override {
    std::cout << "Tearing down simple PingPong test..." << std::endl;
    harness_.stop_all();
    // Note: Event collector not started, so no need to stop it
  }

  void run_tests() override {
    add_test("test_basic_ping_pong", [this]() { test_basic_ping_pong(); });

    // Don't call run_all_tests() here - it will be called by TestRunner
  }

private:
  void test_basic_ping_pong() {
    // Test that sequencer is running
    assert_true(harness_.is_running(test_framework::ApplicationType::SEQUENCER),
                "Sequencer should be running");

    // Test that we can send a command (this tests the command interface)
    assert_true(harness_.get_command_interface().send_text_command("PING", 2, 1),
                "Should be able to send ping command");

    // Wait a moment for the command to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "✓ Basic test passed!" << std::endl;
  }
};

} // namespace test_framework
