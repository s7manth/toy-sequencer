#include "ping.hpp"

#include <chrono>
#include <thread>

PingApp::PingApp(CommandBus &bus, std::function<void(const std::string &)> log,
                 uint64_t instance_id)
    : bus_(bus), log_(std::move(log)), instance_id_(instance_id) {}

void PingApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Ping received ack for sequenced PING, seq=" +
         std::to_string(event.seq()));
  }
}

void PingApp::run() {
  toysequencer::TextCommand cmd;
  cmd.set_text("PING");
  cmd.set_tin(instance_id_); // Set target instance ID (pinging itself)
  send_command(cmd, instance_id_);
  log_("Ping sent PING");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void PingApp::send_command(const toysequencer::TextCommand &command,
                           uint64_t sender_id) {
  bus_.publish(command, sender_id);
}
