#include "pong.hpp"

PongApp::PongApp(std::function<void(const std::string &)> log)
    : log_(std::move(log)) {}

void PongApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Pong received message from Ping, seq=" + std::to_string(event.seq()));
  }
}