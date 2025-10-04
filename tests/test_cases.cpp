#include "test_suite.hpp"
#include <iostream>
#include <thread>
#include <cassert>

namespace test_framework {

class PingPongTestSuite : public TestSuite {
public:
  PingPongTestSuite() : TestSuite("PingPong Integration Tests") {}

  void setup() override {
    std::cout << "Setting up PingPong test environment..." << std::endl;

    assert(harness_.start_sequencer());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    assert(harness_.start_pong(2, 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    assert(harness_.start_ping(1, 2));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

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
    harness_.get_event_collector().clear_all_events();

    assert(harness_.get_command_interface().send_text_command("PING", 2, 1));
    assert(harness_.wait_for_event_count<toysequencer::TextEvent>(2, std::chrono::milliseconds(5000)));

    auto events = harness_.get_recent_events<toysequencer::TextEvent>(2);
    assert("2" == std::to_string(events.size()));
    assert("PING" == events[0].text());
    assert("1" == std::to_string(events[0].sid()));
    assert("PONG" == events[1].text());
    assert("2" == std::to_string(events[1].sid()));
  }

  void test_ping_pong_multiple_messages() {
    harness_.get_event_collector().clear_all_events();

    for (int i = 0; i < 3; ++i) {
      assert(harness_.get_command_interface().send_text_command("PING", 2, 1));
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    assert(harness_.wait_for_event_count<toysequencer::TextEvent>(6, std::chrono::milliseconds(10000)));

    auto events = harness_.get_recent_events<toysequencer::TextEvent>(6);
    assert(6 == events.size());
    for (size_t i = 0; i < events.size(); i += 2) {
      assert("PING" == events[i].text());
      assert("PONG" == events[i + 1].text());
    }
  }

  void test_event_sequencing() {
    harness_.get_event_collector().clear_all_events();

    assert(harness_.get_command_interface().send_text_command("PING", 2, 1));
    assert(harness_.wait_for_event_count<toysequencer::TextEvent>(2, std::chrono::milliseconds(5000)));
    auto events = harness_.get_recent_events<toysequencer::TextEvent>(2);
    assert(events[0].seq() < events[1].seq());
    assert(events[0].timestamp() <= events[1].timestamp());
  }
};

class MarketDataTestSuite : public TestSuite {
public:
  MarketDataTestSuite() : TestSuite("Market Data Integration Tests") {}

  void setup() override {
    std::cout << "Setting up Market Data test environment..." << std::endl;

    assert(harness_.start_sequencer());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    assert(harness_.start_market_data(3));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    assert(harness_.start_scrappy("test_market_data_events.txt"));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

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
    assert(harness_.get_command_interface().send_top_of_book_command("AAPL", 150.25, 100,150.30, 200, 3));
    assert(harness_.wait_for_event_count<toysequencer::TopOfBookEvent>(1, std::chrono::milliseconds(5000)));
    auto events = harness_.get_recent_events<toysequencer::TopOfBookEvent>(1);
    assert(1 == events.size());
    auto &event = events[0];
    assert("AAPL" == event.symbol());
    assert(std::abs(event.bid_price() - 150.25) < 0.001);
    assert(100 == event.bid_size());
    assert(std::abs(event.ask_price() - 150.30) < 0.001);
    assert(200 == event.ask_size());
  }

  void test_market_data_event_processing() {
    harness_.get_event_collector().clear_all_events();

    std::vector<std::string> symbols = {"AAPL", "GOOGL", "MSFT"};
    for (const auto &symbol : symbols) {
      assert(harness_.get_command_interface().send_top_of_book_command(symbol, 100.0, 50,
                                                                            100.5, 75, 3));
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    assert(harness_.wait_for_event_count<toysequencer::TopOfBookEvent>(
                    3, std::chrono::milliseconds(10000)));

    auto events = harness_.get_recent_events<toysequencer::TopOfBookEvent>(3);
    assert(3 == events.size());

    std::vector<std::string> received_symbols;
    for (const auto &event : events) {
      received_symbols.push_back(event.symbol());
    }

    for (const auto &symbol : symbols) {
      assert(std::find(received_symbols.begin(), received_symbols.end(), symbol) !=
                      received_symbols.end());
    }
  }
};

class FullSystemTestSuite : public TestSuite {
public:
  FullSystemTestSuite() : TestSuite("Full System Integration Tests") {}

  void setup() override {
    std::cout << "Setting up Full System test environment..." << std::endl;

    assert(harness_.start_sequencer());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    assert(harness_.start_pong(2, 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    assert(harness_.start_ping(1, 2));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    assert(harness_.start_market_data(3));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    assert(harness_.start_scrappy("test_full_system_events.txt"));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

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

    assert(harness_.get_command_interface().send_text_command("PING", 2, 1));
    assert(harness_.get_command_interface().send_top_of_book_command("AAPL", 150.0, 100, 150.5, 200, 3));

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto text_events = harness_.get_recent_events<toysequencer::TextEvent>(10);
    auto tob_events = harness_.get_recent_events<toysequencer::TopOfBookEvent>(10);

    assert(2 >= text_events.size());
    assert(1 >= tob_events.size());

    bool found_ping = false, found_pong = false;
    for (const auto &event : text_events) {
      if (event.text() == "PING")
        found_ping = true;
      if (event.text() == "PONG")
        found_pong = true;
    }
    assert(found_ping);
    assert(found_pong);

    assert("AAPL" == tob_events[0].symbol());
  }

  void test_system_load() {
    harness_.get_event_collector().clear_all_events();

    for (int i = 0; i < 10; ++i) {
      assert(harness_.get_command_interface().send_text_command("PING", 2, 1));

      assert(harness_.get_command_interface().send_top_of_book_command(
                      "SYMBOL" + std::to_string(i), 100.0 + i, 100, 100.5 + i, 200, 3));

      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    auto text_events = harness_.get_recent_events<toysequencer::TextEvent>(50);
    auto tob_events = harness_.get_recent_events<toysequencer::TopOfBookEvent>(50);

    assert(20 >= text_events.size());
    assert(10 >= tob_events.size());

    std::cout << "Processed " << text_events.size() << " text events and " << tob_events.size()
              << " top of book events" << std::endl;
  }
};

}
