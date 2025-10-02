#include "pong.hpp"

PongApp::PongApp(CommandBus &bus, std::function<void(const std::string &)> log,
                 uint64_t instance_id, uint64_t ping_instance_id)
    : EventReceiver(instance_id), bus_(bus), log_(std::move(log)),
      ping_instance_id_(ping_instance_id) {}

void PongApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Pong received message from Ping, seq=" + std::to_string(event.seq()));

    // Send PONG response back to Ping
    toysequencer::TextCommand cmd;
    cmd.set_text("PONG");
    cmd.set_tin(ping_instance_id_); // Send back to Ping
    send_command(cmd, get_instance_id());
    log_("Pong sent PONG response to Ping");
  }
}

void PongApp::send_command(const toysequencer::TextCommand &command,
                           uint64_t sender_id) {
  bus_.publish(command, sender_id);
}