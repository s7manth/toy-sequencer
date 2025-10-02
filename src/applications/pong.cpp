#include "pong.hpp"

PongApp::PongApp(CommandBus &bus, std::function<void(const std::string &)> log,
                 uint64_t instance_id)
    : bus_(bus), log_(std::move(log)), instance_id_(instance_id) {}

void PongApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Pong received message from Ping, seq=" + std::to_string(event.seq()));
  }
}

void PongApp::send_command(const toysequencer::TextCommand &command,
                           uint64_t sender_id) {
  bus_.publish(command, sender_id);
}