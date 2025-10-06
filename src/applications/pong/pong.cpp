#include "pong.hpp"
#include "utils/instanceid_utils.hpp"

PongApp::PongApp(const std::string &cmd_multicast_address, const uint16_t cmd_port, const uint8_t ttl,
                 const std::string &events_multicast_address, const uint16_t events_port,
                 const std::function<void(const std::string &)> log)
    : ICommandSender<PongApp>(cmd_multicast_address, cmd_port, ttl),
      EventReceiver<PongApp>(get_instance_id(), events_multicast_address, events_port), log_(std::move(log)) {}

void PongApp::on_event(const toysequencer::TextEvent &event) {
  if (event.tin() != get_instance_id())
    return;

  log_("Pong received message from Ping, seq=" + std::to_string(event.seq()));

  toysequencer::TextCommand cmd;
  cmd.set_msg_type(toysequencer::TEXT_COMMAND);
  cmd.set_text("PONG");
  cmd.set_tin(InstanceIdUtils::get_instance_id("PING"));
  cmd.set_sid(get_instance_id());
  this->send_command(cmd, get_instance_id());
  log_("Pong sent PONG response to Ping");
}

void PongApp::send_command(const toysequencer::TextCommand &command, uint64_t sender_id) {
  std::string bytes = command.SerializeAsString();
  std::vector<uint8_t> data(bytes.begin(), bytes.end());
  this->send_m(data);
}

void PongApp::start() { EventReceiver<PongApp>::start(); }

void PongApp::stop() { EventReceiver<PongApp>::stop(); }

uint64_t PongApp::get_instance_id() const { return InstanceIdUtils::get_instance_id("PONG"); }
