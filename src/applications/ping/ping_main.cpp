#include "../../utils/env_utils.hpp"
#include "../../utils/instanceid_utils.hpp"
#include "ping.hpp"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

static std::atomic<bool> running{true};
static void handle_signal(int) { running.store(false); }

int main() {
  try {
    EnvUtils::load_env();

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    auto log = [](const std::string &s) { std::cout << s << std::endl; };

    const std::string events_addr = std::getenv("EVENTS_ADDR");
    const uint16_t events_port = std::stoi(std::getenv("EVENTS_PORT"));
    const std::string cmd_addr = std::getenv("CMD_ADDR");
    const uint16_t cmd_port = std::stoi(std::getenv("CMD_PORT"));
    const uint8_t mcast_ttl = 1;

    PingApp ping(cmd_addr, cmd_port, 1, events_addr, events_port, log);
    ping.subscribe<toysequencer::TextEvent>(toysequencer::TEXT_EVENT);

    ping.start();

    std::cout << "ping sending commands to " << cmd_addr << ":" << cmd_port << ", listening for events on "
              << events_addr << ":" << events_port << std::endl;

    auto cmd = toysequencer::TextCommand();
    cmd.set_msg_type(toysequencer::TEXT_COMMAND);
    cmd.set_text("PING");
    cmd.set_tin(InstanceIdUtils::get_instance_id("PONG"));
    cmd.set_sid(ping.get_instance_id());
    ping.send_command(cmd, ping.get_instance_id());

    while (running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    ping.stop();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "ping error: " << e.what() << std::endl;
    return 1;
  }
}
