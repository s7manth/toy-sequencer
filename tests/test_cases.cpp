#include "test_suite.hpp"
#include <iostream>
#include <thread>

namespace test_framework {

class PingPongTestSuite : public TestSuite {
public:
  PingPongTestSuite() : TestSuite("PingPong Integration Tests") {}

  void setup() override {
    std::cout << "Setting up PingPong test environment..." << std::endl;

    // Start sequencer first
    assert_true(harness_.start_sequencer(), "Failed to start sequencer");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Start pong before ping (pong needs to be ready to receive)
    assert_true(harness_.start_pong(2, 1), "Failed to start pong");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Start ping
    assert_true(harness_.start_ping(1, 2), "Failed to start ping");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Start event collection
    harness_.get_event_collector().start();
    harness_.get_event_collector().subscribe<toysequencer::TextEvent>();

    std::cout << "PingPong test environment ready" << std::endl;
  }

  void teardown() override {
    std::cout << "Tearing down PingPong test environment..." << std::endl;
    harness_.stop_all();
    harness_.get_event_collector().stop();
  }

  void run_tests() override {
    add_test("test_ping_pong_basic_communication",
             [this]() { test_ping_pong_basic_communication(); });

    add_test("test_ping_pong_multiple_messages", [this]() { test_ping_pong_multiple_messages(); });

    add_test("test_event_sequencing", [this]() { test_event_sequencing(); });

    run_all_tests();
  }

private:
  void test_ping_pong_basic_communication() {
    // Clear any existing events
    harness_.get_event_collector().clear_all_events();

    // Send a ping command
    assert_true(harness_.get_command_interface().send_text_command("PING", 2, 1),
                "Failed to send ping command");

    // Wait for events (ping -> pong)
    assert_true(
        harness_.wait_for_event_count<toysequencer::TextEvent>(2, std::chrono::milliseconds(5000)),
        "Did not receive expected ping-pong events");

    auto events = harness_.get_recent_events<toysequencer::TextEvent>(2);
    assert_equals("2", std::to_string(events.size()), "Expected exactly 2 events");

    // Verify ping event
    assert_equals("PING", events[0].text(), "First event should be PING");
    assert_equals("1", std::to_string(events[0].sid()), "Ping sender ID should be 1");

    // Verify pong event
    assert_equals("PONG", events[1].text(), "Second event should be PONG");
    assert_equals("2", std::to_string(events[1].sid()), "Pong sender ID should be 2");
  }

  void test_ping_pong_multiple_messages() {
    harness_.get_event_collector().clear_all_events();

    // Send multiple ping commands
    for (int i = 0; i < 3; ++i) {
      assert_true(harness_.get_command_interface().send_text_command("PING", 2, 1),
                  "Failed to send ping command " + std::to_string(i));
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for all events (3 ping + 3 pong = 6 events)
    assert_true(
        harness_.wait_for_event_count<toysequencer::TextEvent>(6, std::chrono::milliseconds(10000)),
        "Did not receive expected 6 events");

    auto events = harness_.get_recent_events<toysequencer::TextEvent>(6);
    assert_equals("6", std::to_string(events.size()), "Expected exactly 6 events");

    // Verify alternating pattern
    for (size_t i = 0; i < events.size(); i += 2) {
      assert_equals("PING", events[i].text(), "Even indexed events should be PING");
      assert_equals("PONG", events[i + 1].text(), "Odd indexed events should be PONG");
    }
  }

  void test_event_sequencing() {
    harness_.get_event_collector().clear_all_events();

    // Send a ping command
    assert_true(harness_.get_command_interface().send_text_command("PING", 2, 1),
                "Failed to send ping command");

    // Wait for events
    assert_true(
        harness_.wait_for_event_count<toysequencer::TextEvent>(2, std::chrono::milliseconds(5000)),
        "Did not receive expected events");

    auto events = harness_.get_recent_events<toysequencer::TextEvent>(2);

    // Verify sequence numbers are increasing
    assert_true(events[0].seq() < events[1].seq(), "Sequence numbers should be increasing");

    // Verify timestamps are increasing
    assert_true(events[0].timestamp() <= events[1].timestamp(),
                "Timestamps should be non-decreasing");
  }
};

class MarketDataTestSuite : public TestSuite {
public:
  MarketDataTestSuite() : TestSuite("Market Data Integration Tests") {}

  void setup() override {
    std::cout << "Setting up Market Data test environment..." << std::endl;

    // Start sequencer first
    assert_true(harness_.start_sequencer(), "Failed to start sequencer");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Start market data app
    assert_true(harness_.start_market_data(3), "Failed to start market data app");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Start scrappy to capture events
    assert_true(harness_.start_scrappy("test_market_data_events.txt"), "Failed to start scrappy");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Start event collection
    harness_.get_event_collector().start();
    harness_.get_event_collector().subscribe<toysequencer::TopOfBookEvent>();

    std::cout << "Market Data test environment ready" << std::endl;
  }

  void teardown() override {
    std::cout << "Tearing down Market Data test environment..." << std::endl;
    harness_.stop_all();
    harness_.get_event_collector().stop();
  }

  void run_tests() override {
    add_test("test_market_data_command_sending", [this]() { test_market_data_command_sending(); });

    add_test("test_market_data_event_processing",
             [this]() { test_market_data_event_processing(); });

    run_all_tests();
  }

private:
  void test_market_data_command_sending() {
    harness_.get_event_collector().clear_all_events();

    // Send a top of book command
    assert_true(harness_.get_command_interface().send_top_of_book_command("AAPL", 150.25, 100,
                                                                          150.30, 200, 3),
                "Failed to send top of book command");

    // Wait for the event to be processed
    assert_true(harness_.wait_for_event_count<toysequencer::TopOfBookEvent>(
                    1, std::chrono::milliseconds(5000)),
                "Did not receive expected top of book event");

    auto events = harness_.get_recent_events<toysequencer::TopOfBookEvent>(1);
    assert_equals("1", std::to_string(events.size()), "Expected exactly 1 event");

    auto &event = events[0];
    assert_equals("AAPL", event.symbol(), "Symbol should match");
    assert_true(std::abs(event.bid_price() - 150.25) < 0.001, "Bid price should match");
    assert_equals("100", std::to_string(event.bid_size()), "Bid size should match");
    assert_true(std::abs(event.ask_price() - 150.30) < 0.001, "Ask price should match");
    assert_equals("200", std::to_string(event.ask_size()), "Ask size should match");
  }

  void test_market_data_event_processing() {
    harness_.get_event_collector().clear_all_events();

    // Send multiple top of book commands
    std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT"};
    for (const auto &symbol : symbols) {
      assert_true(harness_.get_command_interface().send_top_of_book_command(symbol, 100.0, 50,
                                                                            100.5, 75, 3),
                  "Failed to send top of book command for " + symbol);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for all events
    assert_true(harness_.wait_for_event_count<toysequencer::TopOfBookEvent>(
                    3, std::chrono::milliseconds(10000)),
                "Did not receive expected 3 events");

    auto events = harness_.get_recent_events<toysequencer::TopOfBookEvent>(3);
    assert_equals("3", std::to_string(events.size()), "Expected exactly 3 events");

    // Verify all symbols are present
    std::vector<std::string> received_symbols;
    for (const auto &event : events) {
      received_symbols.push_back(event.symbol());
    }

    for (const auto &symbol : symbols) {
      assert_true(std::find(received_symbols.begin(), received_symbols.end(), symbol) !=
                      received_symbols.end(),
                  "Symbol " + symbol + " not found in events");
    }
  }
};

class FullSystemTestSuite : public TestSuite {
public:
  FullSystemTestSuite() : TestSuite("Full System Integration Tests") {}

  void setup() override {
    std::cout << "Setting up Full System test environment..." << std::endl;

    // Start all applications
    assert_true(harness_.start_sequencer(), "Failed to start sequencer");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    assert_true(harness_.start_pong(2, 1), "Failed to start pong");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    assert_true(harness_.start_ping(1, 2), "Failed to start ping");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    assert_true(harness_.start_market_data(3), "Failed to start market data app");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    assert_true(harness_.start_scrappy("test_full_system_events.txt"), "Failed to start scrappy");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Start event collection for both event types
    harness_.get_event_collector().start();
    harness_.get_event_collector().subscribe<toysequencer::TextEvent>();
    harness_.get_event_collector().subscribe<toysequencer::TopOfBookEvent>();

    std::cout << "Full System test environment ready" << std::endl;
  }

  void teardown() override {
    std::cout << "Tearing down Full System test environment..." << std::endl;
    harness_.stop_all();
    harness_.get_event_collector().stop();
  }

  void run_tests() override {
    add_test("test_mixed_event_types", [this]() { test_mixed_event_types(); });

    add_test("test_system_load", [this]() { test_system_load(); });

    run_all_tests();
  }

private:
  void test_mixed_event_types() {
    harness_.get_event_collector().clear_all_events();

    // Send both text and market data commands
    assert_true(harness_.get_command_interface().send_text_command("PING", 2, 1),
                "Failed to send ping command");

    assert_true(harness_.get_command_interface().send_top_of_book_command("AAPL", 150.0, 100, 150.5,
                                                                          200, 3),
                "Failed to send top of book command");

    // Wait for events (ping + pong + top of book = 3 events)
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto text_events = harness_.get_recent_events<toysequencer::TextEvent>(10);
    auto tob_events = harness_.get_recent_events<toysequencer::TopOfBookEvent>(10);

    assert_greater_equal(2, text_events.size(), "Should have at least 2 text events (ping/pong)");
    assert_greater_equal(1, tob_events.size(), "Should have at least 1 top of book event");

    // Verify text events
    bool found_ping = false, found_pong = false;
    for (const auto &event : text_events) {
      if (event.text() == "PING")
        found_ping = true;
      if (event.text() == "PONG")
        found_pong = true;
    }
    assert_true(found_ping, "Should have found PING event");
    assert_true(found_pong, "Should have found PONG event");

    // Verify top of book event
    assert_equals("AAPL", tob_events[0].symbol(), "Top of book symbol should be AAPL");
  }

  void test_system_load() {
    harness_.get_event_collector().clear_all_events();

    // Send many commands rapidly
    for (int i = 0; i < 10; ++i) {
      assert_true(harness_.get_command_interface().send_text_command("PING", 2, 1),
                  "Failed to send ping command " + std::to_string(i));

      assert_true(harness_.get_command_interface().send_top_of_book_command(
                      "SYMBOL" + std::to_string(i), 100.0 + i, 100, 100.5 + i, 200, 3),
                  "Failed to send top of book command " + std::to_string(i));

      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Wait for all events to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    auto text_events = harness_.get_recent_events<toysequencer::TextEvent>(50);
    auto tob_events = harness_.get_recent_events<toysequencer::TopOfBookEvent>(50);

    // Should have at least 20 text events (10 ping + 10 pong) and 10 top of book events
    assert_greater_equal(20, text_events.size(), "Should have processed many text events");
    assert_greater_equal(10, tob_events.size(), "Should have processed many top of book events");

    std::cout << "Processed " << text_events.size() << " text events and " << tob_events.size()
              << " top of book events" << std::endl;
  }
};

} // namespace test_framework
