#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../src/generated/messages.pb.h"

class MulticastSender;

namespace test_framework {

uint16_t find_available_port(uint16_t start_port = 30001);

enum class ApplicationType { SEQUENCER, PING, PONG, SCRAPPY, MARKET_DATA };

struct ApplicationConfig {
  ApplicationType type;
  std::string executable_path;
  std::vector<std::string> args;
  uint64_t instance_id;
  std::unordered_map<std::string, std::string> env_vars;
  std::chrono::milliseconds startup_timeout{5000};
  std::chrono::milliseconds shutdown_timeout{2000};
};

class ApplicationManager {
public:
  ApplicationManager() = default;
  ~ApplicationManager() { stop_all(); }

  bool start_application(const ApplicationConfig &config);
  bool stop_application(ApplicationType type);
  bool stop_all();

  bool is_running(ApplicationType type) const;
  std::string get_output(ApplicationType type) const;
  std::string get_error(ApplicationType type) const;

  void wait_for_startup(ApplicationType type,
                        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
  void wait_for_shutdown(ApplicationType type,
                         std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));

private:
  struct ApplicationProcess {
    pid_t pid = -1;
    ApplicationConfig config;
    std::string output;
    std::string error;
    std::atomic<bool> running{false};
    std::thread output_thread;
    std::thread error_thread;
    int stdout_pipe[2] = {-1, -1};
    int stderr_pipe[2] = {-1, -1};
  };

  mutable std::mutex mutex_;
  std::unordered_map<ApplicationType, std::unique_ptr<ApplicationProcess>> processes_;

  void collect_output(ApplicationProcess *app);
  void collect_error(ApplicationProcess *app);
  bool create_pipes(ApplicationProcess *app);
  void close_pipes(ApplicationProcess *app);
};

class EventCollector {
public:
  EventCollector(const std::string &multicast_addr = "239.255.0.1", uint16_t port = 30001);
  ~EventCollector();

  void start();
  void stop();

  template <typename EventT> void subscribe();

  template <typename EventT> std::vector<EventT> get_events() const;

  template <typename EventT> void clear_events();

  void clear_all_events();

  bool wait_for_event(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

  // Get the actual port being used (for dynamic allocation)
  uint16_t get_port() const { return port_; }

private:
  void on_datagram(const uint8_t *data, size_t len);

  std::string multicast_addr_;
  uint16_t port_;
  std::atomic<bool> running_{false};
  std::thread receiver_thread_;

  mutable std::mutex events_mutex_;
  std::vector<toysequencer::TextEvent> text_events_;
  std::vector<toysequencer::TopOfBookEvent> tob_events_;

  std::condition_variable event_cv_;
  std::mutex event_cv_mutex_;
};

class CommandInterface {
public:
  CommandInterface(const std::string &multicast_addr = "239.255.0.2", uint16_t port = 30002);
  ~CommandInterface();

  bool send_text_command(const std::string &text, uint64_t target_instance_id,
                         uint64_t sender_instance_id);
  bool send_top_of_book_command(const std::string &symbol, double bid_price, uint32_t bid_size,
                                double ask_price, uint32_t ask_size, uint64_t sender_instance_id);

  // Get the actual port being used (for dynamic allocation)
  uint16_t get_port() const { return port_; }

private:
  std::string multicast_addr_;
  uint16_t port_;
  std::unique_ptr<class MulticastSender> sender_;
};

class TestHarness {
public:
  TestHarness();
  ~TestHarness();

  // Get the ports being used by this test harness
  uint16_t get_events_port() const { return events_port_; }
  uint16_t get_commands_port() const { return commands_port_; }

  // Application management
  bool start_sequencer();
  bool start_ping(uint64_t instance_id = 1, uint64_t pong_instance_id = 2);
  bool start_pong(uint64_t instance_id = 2, uint64_t ping_instance_id = 1);
  bool start_scrappy(const std::string &output_file = "test_sequenced_events.txt");
  bool start_market_data(uint64_t instance_id = 3, const std::string &host = "127.0.0.1",
                         const std::string &port = "8000");

  bool stop_application(ApplicationType type);
  bool stop_all();

  bool is_running(ApplicationType type) const;

  // Get application output and error messages
  std::string get_output(ApplicationType type) const;
  std::string get_error(ApplicationType type) const;

  // Event collection
  EventCollector &get_event_collector() { return event_collector_; }

  // Command interface
  CommandInterface &get_command_interface() { return command_interface_; }

  // Utility methods
  void wait_for_all_startup(std::chrono::milliseconds timeout = std::chrono::milliseconds(10000));
  void wait_for_all_shutdown(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

  template <typename EventT>
  bool wait_for_event_count(size_t expected_count,
                            std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

  template <typename EventT> std::vector<EventT> get_recent_events(size_t count = 10) const;

private:
  ApplicationManager app_manager_;
  EventCollector event_collector_;
  CommandInterface command_interface_;

  std::string build_dir_;
  uint16_t events_port_;
  uint16_t commands_port_;

  std::string get_executable_path(const std::string &app_name) const;
};

// Template implementations
template <typename EventT> void EventCollector::subscribe() {
  // Implementation will be in .cpp file
}

template <typename EventT> std::vector<EventT> EventCollector::get_events() const {
  std::lock_guard<std::mutex> lock(events_mutex_);
  if constexpr (std::is_same_v<EventT, toysequencer::TextEvent>) {
    return text_events_;
  } else if constexpr (std::is_same_v<EventT, toysequencer::TopOfBookEvent>) {
    return tob_events_;
  }
  return {};
}

template <typename EventT> void EventCollector::clear_events() {
  std::lock_guard<std::mutex> lock(events_mutex_);
  if constexpr (std::is_same_v<EventT, toysequencer::TextEvent>) {
    text_events_.clear();
  } else if constexpr (std::is_same_v<EventT, toysequencer::TopOfBookEvent>) {
    tob_events_.clear();
  }
}

template <typename EventT>
bool TestHarness::wait_for_event_count(size_t expected_count, std::chrono::milliseconds timeout) {
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < timeout) {
    auto events = event_collector_.get_events<EventT>();
    if (events.size() >= expected_count) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return false;
}

template <typename EventT> std::vector<EventT> TestHarness::get_recent_events(size_t count) const {
  auto events = event_collector_.get_events<EventT>();
  if (events.size() <= count) {
    return events;
  }
  return std::vector<EventT>(events.end() - count, events.end());
}

}
