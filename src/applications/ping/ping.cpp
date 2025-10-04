#include "ping.hpp"

PingApp::PingApp(const std::string &multicast_address, uint16_t port, uint8_t ttl,
                 std::function<void(const std::string &)> log,
                 uint64_t instance_id, uint64_t pong_instance_id)
    : ICommandSender<PingApp>(multicast_address, port, ttl),
      EventReceiver<PingApp>(instance_id, multicast_address, port), log_(std::move(log)),
      pong_instance_id_(pong_instance_id) {}

void PingApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Ping received ack for sequenced PING, seq=" +
         std::to_string(event.seq()));
  }
}

void PingApp::send_command(const toysequencer::TextCommand &command,
                           uint64_t sender_id) {
  std::string bytes = command.SerializeAsString();
  std::vector<uint8_t> data(bytes.begin(), bytes.end());
  this->send_m(data);
}
