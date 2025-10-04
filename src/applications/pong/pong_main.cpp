#include "pong.hpp"
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

    PongApp pong(cmd_addr, cmd_port, 1, events_addr, events_port, log, pong_instance_id, ping_instance_id);

    pong.start();

    std::cout << "pong listening for TextEvent on " << cmd_addr << ":" << cmd_port << ", instance=" << pong_instance_id
              << std::endl;

    while (running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    pong.stop();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "pong error: " << e.what() << std::endl;
    return 1;
  }
}
