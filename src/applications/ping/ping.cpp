#include "ping.hpp"
#include "utils/instanceid_utils.hpp"

PingApp::PingApp(const std::string &commands_multicast_address, const uint16_t commands_port, const uint8_t ttl,
                 const std::string &events_multicast_address, const uint16_t events_port,
                 const std::function<void(const std::string &)> log)
    : ICommandSender<PingApp>(commands_multicast_address, commands_port, ttl),
      EventReceiver<PingApp>(get_instance_id(), events_multicast_address, events_port), log_(std::move(log)) {}

void PingApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Ping received ack for sequenced PING, seq=" + std::to_string(event.seq()));
  }
}

void PingApp::send_command(const toysequencer::TextCommand &command, uint64_t sender_id) {
  std::string bytes = command.SerializeAsString();
  std::vector<uint8_t> data(bytes.begin(), bytes.end());
  this->send_m(data);
}

void PingApp::start() { EventReceiver<PingApp>::start(); }

void PingApp::stop() { EventReceiver<PingApp>::stop(); }

uint64_t PingApp::get_instance_id() const { return InstanceIdUtils::get_instance_id("PING"); }
