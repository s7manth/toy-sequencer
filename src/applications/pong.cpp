#include "pong.hpp"

PongApp::PongApp(CommandBus &bus, std::function<void(const std::string &)> log)
    : bus_(bus), log_(std::move(log)) {}

void PongApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Pong received message from Ping, seq=" + std::to_string(event.seq()));
  }
}

void PongApp::send_command(const toysequencer::TextCommand &command) {
  bus_.publish(command);
}