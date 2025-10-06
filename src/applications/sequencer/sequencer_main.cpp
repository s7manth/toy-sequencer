#include "../../utils/env_utils.hpp"
#include "messages.pb.h"
#include "sequencer.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

static std::atomic<bool> running{true};
static void handle_signal(int) { running.store(false); }

int main() {
  try {
    EnvUtils::load_env();

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    const std::string events_addr = std::getenv("EVENTS_ADDR");
    const uint16_t events_port = std::stoi(std::getenv("EVENTS_PORT"));
    const std::string cmd_addr = std::getenv("CMD_ADDR");
    const uint16_t cmd_port = std::stoi(std::getenv("CMD_PORT"));
    const uint8_t mcast_ttl = 1;

    Sequencer sequencer(cmd_addr, cmd_port, events_addr, events_port, mcast_ttl);

    sequencer.subscribe<toysequencer::TextCommand>(toysequencer::TEXT_COMMAND);
    sequencer.subscribe<toysequencer::TopOfBookCommand>(toysequencer::TOB_COMMAND);

    sequencer.start();
    std::cout << "sequencer started, listening for commands on " << cmd_addr << ":" << cmd_port
              << " and publishing events to " << events_addr << ":" << events_port << std::endl;

    while (running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    sequencer.stop();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "sequencer error: " << e.what() << std::endl;
    return 1;
  }
}
