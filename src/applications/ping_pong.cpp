#include "ping_pong.hpp"

#include <chrono>
#include <iostream>
#include <thread>

PingApp::PingApp(CommandBus &bus, std::function<void(const std::string &)> log)
    : bus_(bus), log_(std::move(log)) {}

PingApp::~PingApp() { stop(); }

void PingApp::start() {
  if (running_.exchange(true)) {
    return;
  }
  thread_ = std::thread([this] { this->run(); });
}

void PingApp::stop() {
  if (!running_.exchange(false)) {
    return;
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

void PingApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Ping received ack for sequenced PING, seq=" +
         std::to_string(event.seq()));
  }
}

void PingApp::run() {
  // simple: send one PING and exit
  toysequencer::TextCommand cmd;
  cmd.set_text("PING");
  bus_.publish(cmd);
  // Give the system a little time to process
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

PongApp::PongApp(std::function<void(const std::string &)> log)
    : log_(std::move(log)) {}

void PongApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Pong received message from Ping, seq=" + std::to_string(event.seq()));
  }
}
