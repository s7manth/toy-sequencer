#include "ping.hpp"

PingApp::PingApp(CommandBus &bus, std::function<void(const std::string &)> log,
                 uint64_t instance_id, uint64_t pong_instance_id)
    : EventReceiver(instance_id), bus_(bus), log_(std::move(log)),
      pong_instance_id_(pong_instance_id) {}

void PingApp::on_event(const toysequencer::TextEvent &event) {
  if (event.text() == "PING") {
    log_("Ping received ack for sequenced PING, seq=" +
         std::to_string(event.seq()));
  }
}

void PingApp::run() {
  toysequencer::TextCommand cmd;
  cmd.set_text("PING1");
  cmd.set_tin(pong_instance_id_); // Set target instance ID (pinging pong)
  send_command(cmd, get_instance_id());
  log_("Ping sent PING 1 to Pong");

  toysequencer::TextCommand cmd2;
  cmd2.set_text("PING2");
  cmd2.set_tin(pong_instance_id_); // Set target instance ID (pinging pong)
  send_command(cmd2, get_instance_id());
  log_("Ping sent PING 2 to Pong");

  // std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void PingApp::send_command(const toysequencer::TextCommand &command,
                           uint64_t sender_id) {
  bus_.publish(command, sender_id);
}
