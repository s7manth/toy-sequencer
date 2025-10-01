#include "ping.hpp"

#include <chrono>
#include <thread>

PingApp::PingApp(CommandBus &bus, std::function<void(const std::string &)> log)
    : bus_(bus), log_(std::move(log)) {}

void PingApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Ping received ack for sequenced PING, seq=" +
         std::to_string(event.seq()));
  }
}

void PingApp::run() {
  toysequencer::TextCommand cmd;
  cmd.set_text("PING");
  bus_.publish(cmd);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
