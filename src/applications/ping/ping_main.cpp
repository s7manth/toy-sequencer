#include "ping.hpp"
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

static std::atomic<bool> running{true};
static void handle_signal(int) { running.store(false); }

int main() {
  try {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    auto log = [](const std::string &s) { std::cout << s << std::endl; };

    const std::string cmd_addr = "239.255.0.2"; // commands to sequencer
    const uint16_t cmd_port = 30002;
    const std::string events_addr = "239.255.0.1"; // events from sequencer
    const uint16_t events_port = 30001;

    uint64_t ping_instance_id = 18;
    uint64_t pong_instance_id = 81;

    PingApp ping(cmd_addr, cmd_port, 1, events_addr, events_port, log, ping_instance_id, pong_instance_id);

    ping.start();
    std::cout << "ping sending commands to " << cmd_addr << ":" << cmd_port << ", listening for events on "
              << events_addr << ":" << events_port << ", instance=" << ping_instance_id << std::endl;

    auto cmd = toysequencer::TextCommand();
    cmd.set_text("PING");
    cmd.set_tin(pong_instance_id);
    ping.send_command(cmd, ping_instance_id);

    ping.stop();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "ping error: " << e.what() << std::endl;
    return 1;
  }
}
