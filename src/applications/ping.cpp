#include "ping.hpp"

PingApp::PingApp(CommandBus &bus, std::function<void(const std::string &)> log,
                 uint64_t instance_id, uint64_t pong_instance_id)
    : EventReceiver<PingApp>(instance_id), bus_(bus), log_(std::move(log)),
      pong_instance_id_(pong_instance_id) {}

void PingApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Ping received ack for sequenced PING, seq=" +
         std::to_string(event.seq()));
  }
}

void PingApp::send_command(const toysequencer::TextCommand &command,
                           uint64_t sender_id) {
  bus_.publish(command, sender_id);
}
