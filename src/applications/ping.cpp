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
  send_command(cmd);
  log_("Ping sent PING");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void PingApp::send_command(const toysequencer::TextCommand &command) {
  bus_.publish(command);
}
