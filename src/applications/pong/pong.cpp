#include "pong.hpp"

PongApp::PongApp(const std::string &multicast_address, uint16_t port, uint8_t ttl, 
                 CommandBus &bus, std::function<void(const std::string &)> log,
                 uint64_t instance_id, uint64_t ping_instance_id)
    : ICommandSender<PongApp>(multicast_address, port, ttl),
      EventReceiver<PongApp>(instance_id), bus_(bus), log_(std::move(log)),
      ping_instance_id_(ping_instance_id) {}

void PongApp::on_event(const toysequencer::TextEvent &event) {
  log_("Pong received message from Ping, seq=" + std::to_string(event.seq()));

  toysequencer::TextCommand cmd;
  cmd.set_text("PONG");
  cmd.set_tin(ping_instance_id_);
  this->send(cmd, get_instance_id());
  log_("Pong sent PONG response to Ping");
}

void PongApp::send_command(const toysequencer::TextCommand &command,
                           uint64_t sender_id) {
  bus_.publish(command, sender_id);
}