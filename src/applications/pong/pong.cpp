#include "pong.hpp"

PongApp::PongApp(const std::string &cmd_multicast_address, 
                 const uint16_t cmd_port, 
                 const uint8_t ttl,
                 const std::string &events_multicast_address, 
                 const uint16_t events_port,
                 const std::function<void(const std::string &)> log,
                 const uint64_t instance_id,
                 const uint64_t ping_instance_id)
    : ICommandSender<PongApp>(cmd_multicast_address, cmd_port, ttl),
      EventReceiver<PongApp>(instance_id, events_multicast_address, events_port), log_(std::move(log)),
      ping_instance_id_(ping_instance_id) {}

void PongApp::on_event(const toysequencer::TextEvent &event) {
  log_("Pong received message from Ping, seq=" + std::to_string(event.seq()));

  toysequencer::TextCommand cmd;
  cmd.set_text("PONG");
  cmd.set_tin(ping_instance_id_);
  this->send(cmd, get_instance_id());
  log_("Pong sent PONG response to Ping");
}

void PongApp::send_command(const toysequencer::TextCommand &command, uint64_t sender_id) {
  std::string bytes = command.SerializeAsString();
  std::vector<uint8_t> data(bytes.begin(), bytes.end());
  this->send_m(data);
}