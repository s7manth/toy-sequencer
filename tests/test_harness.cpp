#include "test_harness.hpp"
#include "../src/core/multicast_receiver.hpp"
#include "../src/core/multicast_sender.hpp"
#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace test_framework {

uint16_t find_available_port(uint16_t start_port) {
  for (uint16_t port = start_port; port < start_port + 1000; ++port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
      continue;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
      close(sock);
      return port;
    }
    close(sock);
  }
  throw std::runtime_error("Could not find available port");
}

bool ApplicationManager::start_application(const ApplicationConfig &config) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (processes_.find(config.type) != processes_.end()) {
    std::cerr << "Application " << static_cast<int>(config.type) << " is already running"
              << std::endl;
    return false;
  }

  auto app = std::make_unique<ApplicationProcess>();
  app->config = config;

  if (!create_pipes(app.get())) {
    std::cerr << "Failed to create pipes for application " << static_cast<int>(config.type)
              << std::endl;
    return false;
  }

  pid_t pid = fork();
  if (pid == -1) {
    std::cerr << "Failed to fork process for application " << static_cast<int>(config.type)
              << std::endl;
    close_pipes(app.get());
    return false;
  }

  if (pid == 0) {
    // Child process
    dup2(app->stdout_pipe[1], STDOUT_FILENO);
    dup2(app->stderr_pipe[1], STDERR_FILENO);

    close_pipes(app.get());

    // Prepare arguments
    std::vector<char *> argv;
    argv.push_back(const_cast<char *>(config.executable_path.c_str()));
    for (const auto &arg : config.args) {
      argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);

    // Execute the application
    execv(config.executable_path.c_str(), argv.data());
    std::cerr << "Failed to execute " << config.executable_path << std::endl;
    exit(1);
  } else {
    // Parent process
    app->pid = pid;
    app->running = true;

    // Close write ends of pipes
    close(app->stdout_pipe[1]);
    close(app->stderr_pipe[1]);

    // Start output collection threads
    app->output_thread = std::thread(&ApplicationManager::collect_output, this, app.get());
    app->error_thread = std::thread(&ApplicationManager::collect_error, this, app.get());

    processes_[config.type] = std::move(app);

    std::cout << "Started application " << static_cast<int>(config.type) << " with PID " << pid
              << std::endl;
    return true;
  }
}

bool ApplicationManager::stop_application(ApplicationType type) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = processes_.find(type);
  if (it == processes_.end()) {
    return true; // Already stopped
  }

  auto &app = it->second;
  app->running = false;

  if (app->pid > 0) {
    kill(app->pid, SIGTERM);

    // Wait for process to terminate
    int status;
    pid_t result = waitpid(app->pid, &status, WNOHANG);
    if (result == 0) {
      // Process still running, force kill
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      kill(app->pid, SIGKILL);
      waitpid(app->pid, &status, 0);
    }

    close_pipes(app.get());

    if (app->output_thread.joinable()) {
      app->output_thread.join();
    }
    if (app->error_thread.joinable()) {
      app->error_thread.join();
    }

    processes_.erase(it);
    std::cout << "Stopped application " << static_cast<int>(type) << std::endl;
    return true;
  }

  return false;
}

bool ApplicationManager::stop_all() {
  std::lock_guard<std::mutex> lock(mutex_);
  bool all_success = true;

  for (auto &[type, app] : processes_) {
    app->running = false;
    if (app->pid > 0) {
      kill(app->pid, SIGTERM);

      int status;
      pid_t result = waitpid(app->pid, &status, WNOHANG);
      if (result == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        kill(app->pid, SIGKILL);
        waitpid(app->pid, &status, 0);
      }

      close_pipes(app.get());

      if (app->output_thread.joinable()) {
        app->output_thread.join();
      }
      if (app->error_thread.joinable()) {
        app->error_thread.join();
      }
    }
  }

  processes_.clear();
  return all_success;
}

bool ApplicationManager::is_running(ApplicationType type) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = processes_.find(type);
  if (it == processes_.end()) {
    return false;
  }

  // Check if process is still alive
  if (it->second->pid > 0) {
    int status;
    pid_t result = waitpid(it->second->pid, &status, WNOHANG);
    if (result == 0) {
      return it->second->running.load();
    } else {
      return false; // Process has terminated
    }
  }

  return false;
}

std::string ApplicationManager::get_output(ApplicationType type) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = processes_.find(type);
  return it != processes_.end() ? it->second->output : "";
}

std::string ApplicationManager::get_error(ApplicationType type) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = processes_.find(type);
  return it != processes_.end() ? it->second->error : "";
}

void ApplicationManager::wait_for_startup(ApplicationType type, std::chrono::milliseconds timeout) {
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < timeout) {
    if (is_running(type)) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  throw std::runtime_error("Application startup timeout");
}

void ApplicationManager::wait_for_shutdown(ApplicationType type,
                                           std::chrono::milliseconds timeout) {
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < timeout) {
    if (!is_running(type)) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  throw std::runtime_error("Application shutdown timeout");
}

bool ApplicationManager::create_pipes(ApplicationProcess *app) {
  if (pipe(app->stdout_pipe) == -1) {
    return false;
  }
  if (pipe(app->stderr_pipe) == -1) {
    close(app->stdout_pipe[0]);
    close(app->stdout_pipe[1]);
    return false;
  }
  return true;
}

void ApplicationManager::close_pipes(ApplicationProcess *app) {
  if (app->stdout_pipe[0] != -1)
    close(app->stdout_pipe[0]);
  if (app->stdout_pipe[1] != -1)
    close(app->stdout_pipe[1]);
  if (app->stderr_pipe[0] != -1)
    close(app->stderr_pipe[0]);
  if (app->stderr_pipe[1] != -1)
    close(app->stderr_pipe[1]);
}

void ApplicationManager::collect_output(ApplicationProcess *app) {
  char buffer[1024];
  while (app->running.load()) {
    ssize_t bytes_read = read(app->stdout_pipe[0], buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
      buffer[bytes_read] = '\0';
      app->output += buffer;
    } else if (bytes_read == 0) {
      break; // EOF
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}

void ApplicationManager::collect_error(ApplicationProcess *app) {
  char buffer[1024];
  while (app->running.load()) {
    ssize_t bytes_read = read(app->stderr_pipe[0], buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
      buffer[bytes_read] = '\0';
      app->error += buffer;
    } else if (bytes_read == 0) {
      break; // EOF
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}

// EventCollector Implementation
EventCollector::EventCollector(const std::string &multicast_addr, uint16_t port)
    : multicast_addr_(multicast_addr), port_(port) {}

EventCollector::~EventCollector() { stop(); }

void EventCollector::start() {
  if (running_.load()) {
    return;
  }

  running_ = true;
  receiver_thread_ = std::thread([this]() {
    try {
      MulticastReceiver receiver(multicast_addr_, port_);
      receiver.subscribe([this](const uint8_t *data, size_t len) { on_datagram(data, len); });
      receiver.start();

      while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      receiver.stop();
    } catch (const std::exception &e) {
      std::cerr << "EventCollector error: " << e.what() << std::endl;
    }
  });
}

void EventCollector::stop() {
  if (!running_.load()) {
    return;
  }

  running_ = false;
  if (receiver_thread_.joinable()) {
    receiver_thread_.join();
  }
}

template <> void EventCollector::subscribe<toysequencer::TextEvent>() {
  // Already handled in on_datagram
}

template <> void EventCollector::subscribe<toysequencer::TopOfBookEvent>() {
  // Already handled in on_datagram
}

void EventCollector::on_datagram(const uint8_t *data, size_t len) {
  try {
    // Try to parse as TextEvent first
    toysequencer::TextEvent text_event;
    if (text_event.ParseFromArray(data, static_cast<int>(len))) {
      std::lock_guard<std::mutex> lock(events_mutex_);
      text_events_.push_back(text_event);
      event_cv_.notify_all();
      return;
    }

    // Try to parse as TopOfBookEvent
    toysequencer::TopOfBookEvent tob_event;
    if (tob_event.ParseFromArray(data, static_cast<int>(len))) {
      std::lock_guard<std::mutex> lock(events_mutex_);
      tob_events_.push_back(tob_event);
      event_cv_.notify_all();
      return;
    }

    std::cerr << "Failed to parse event from datagram" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "EventCollector::on_datagram error: " << e.what() << std::endl;
  }
}

bool EventCollector::wait_for_event(std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock(event_cv_mutex_);
  return event_cv_.wait_for(lock, timeout, [this]() {
    std::lock_guard<std::mutex> events_lock(events_mutex_);
    return !text_events_.empty() || !tob_events_.empty();
  });
}

void EventCollector::clear_all_events() {
  std::lock_guard<std::mutex> lock(events_mutex_);
  text_events_.clear();
  tob_events_.clear();
}

// CommandInterface Implementation
CommandInterface::CommandInterface(const std::string &multicast_addr, uint16_t port)
    : multicast_addr_(multicast_addr), port_(port) {
  sender_ = std::make_unique<MulticastSender>(multicast_addr_, port_, 1);
}

CommandInterface::~CommandInterface() = default;

bool CommandInterface::send_text_command(const std::string &text, uint64_t target_instance_id,
                                         uint64_t sender_instance_id) {
  try {
    toysequencer::TextCommand cmd;
    cmd.set_text(text);
    cmd.set_tin(target_instance_id);

    std::string bytes = cmd.SerializeAsString();
    std::vector<uint8_t> data(bytes.begin(), bytes.end());

    return sender_->send_m(data);
  } catch (const std::exception &e) {
    std::cerr << "Failed to send text command: " << e.what() << std::endl;
    return false;
  }
}

bool CommandInterface::send_top_of_book_command(const std::string &symbol, double bid_price,
                                                uint32_t bid_size, double ask_price,
                                                uint32_t ask_size, uint64_t sender_instance_id) {
  try {
    toysequencer::TopOfBookCommand cmd;
    cmd.set_symbol(symbol);
    cmd.set_bid_price(bid_price);
    cmd.set_bid_size(bid_size);
    cmd.set_ask_price(ask_price);
    cmd.set_ask_size(ask_size);

    std::string bytes = cmd.SerializeAsString();
    std::vector<uint8_t> data(bytes.begin(), bytes.end());

    return sender_->send_m(data);
  } catch (const std::exception &e) {
    std::cerr << "Failed to send top of book command: " << e.what() << std::endl;
    return false;
  }
}

// TestHarness Implementation
TestHarness::TestHarness()
    : event_collector_("239.255.0.1", 30001), command_interface_("239.255.0.2", 30002) {
  // Try to find build directory
  std::vector<std::string> possible_paths = {"build/src", "../build/src", "../../build/src",
                                             "out/build/Clang 17.0.0 arm64-apple-darwin25.0.0/src"};

  for (const auto &path : possible_paths) {
    if (std::filesystem::exists(path)) {
      build_dir_ = path;
      break;
    }
  }

  if (build_dir_.empty()) {
    std::cerr << "Warning: Could not find build directory, using current directory" << std::endl;
    build_dir_ = ".";
  }

  std::cout << "TestHarness initialized with default addresses" << std::endl;
}

TestHarness::~TestHarness() {
  // Stop event collection first to avoid receiving events during shutdown
  event_collector_.stop();

  // Then stop all applications
  stop_all();
}

std::string TestHarness::get_executable_path(const std::string &app_name) const {
  return build_dir_ + "/" + app_name;
}

bool TestHarness::start_sequencer() {
  ApplicationConfig config;
  config.type = ApplicationType::SEQUENCER;
  config.executable_path = get_executable_path("sequencer");
  config.instance_id = 0; // Sequencer doesn't have instance ID

  return app_manager_.start_application(config);
}

bool TestHarness::start_ping(uint64_t instance_id, uint64_t pong_instance_id) {
  ApplicationConfig config;
  config.type = ApplicationType::PING;
  config.executable_path = get_executable_path("ping");
  config.instance_id = instance_id;

  return app_manager_.start_application(config);
}

bool TestHarness::start_pong(uint64_t instance_id, uint64_t ping_instance_id) {
  ApplicationConfig config;
  config.type = ApplicationType::PONG;
  config.executable_path = get_executable_path("pong");
  config.instance_id = instance_id;

  return app_manager_.start_application(config);
}

bool TestHarness::start_scrappy(const std::string &output_file) {
  ApplicationConfig config;
  config.type = ApplicationType::SCRAPPY;
  config.executable_path = get_executable_path("scrappy");
  config.args = {output_file};

  return app_manager_.start_application(config);
}

bool TestHarness::start_market_data(uint64_t instance_id, const std::string &host,
                                    const std::string &port) {
  ApplicationConfig config;
  config.type = ApplicationType::MARKET_DATA;
  config.executable_path = get_executable_path("market_data_app");
  config.instance_id = instance_id;

  return app_manager_.start_application(config);
}

bool TestHarness::stop_application(ApplicationType type) {
  return app_manager_.stop_application(type);
}

bool TestHarness::stop_all() { return app_manager_.stop_all(); }

bool TestHarness::is_running(ApplicationType type) const { return app_manager_.is_running(type); }

std::string TestHarness::get_output(ApplicationType type) const {
  return app_manager_.get_output(type);
}

std::string TestHarness::get_error(ApplicationType type) const {
  return app_manager_.get_error(type);
}

void TestHarness::wait_for_all_startup(std::chrono::milliseconds timeout) {
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < timeout) {
    bool all_running = true;
    for (int i = 0; i < 5; ++i) { // Check all application types
      ApplicationType type = static_cast<ApplicationType>(i);
      if (app_manager_.is_running(type)) {
        continue;
      }
      all_running = false;
      break;
    }
    if (all_running) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  throw std::runtime_error("Not all applications started within timeout");
}

void TestHarness::wait_for_all_shutdown(std::chrono::milliseconds timeout) {
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < timeout) {
    bool all_stopped = true;
    for (int i = 0; i < 5; ++i) { // Check all application types
      ApplicationType type = static_cast<ApplicationType>(i);
      if (app_manager_.is_running(type)) {
        all_stopped = false;
        break;
      }
    }
    if (all_stopped) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  throw std::runtime_error("Not all applications stopped within timeout");
}

} // namespace test_framework