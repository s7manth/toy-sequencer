#include "../adapters.hpp"
#include "../core/command_bus.hpp"
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
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    const std::string mcast_addr = "239.255.0.1"; // events
    const uint16_t events_port = 30001;
    const std::string cmd_addr = "239.255.0.2"; // commands
    const uint16_t cmd_port = 30002;
    const uint8_t mcast_ttl = 1;

    Sequencer sequencer(cmd_addr, cmd_port, mcast_addr, events_port, mcast_ttl);
    CommandBus bus;
    sequencer.attach_command_bus(bus);

    sequencer.subscribe<toysequencer::TextCommand>();
    sequencer.subscribe<toysequencer::TopOfBookCommand>();

    adapters::TextCommandToTextEvent text_adapter;
    adapters::TopOfBookCommandToTopOfBookEvent tob_adapter;
    sequencer.register_pipeline<toysequencer::TextCommand, toysequencer::TextEvent>(text_adapter);
    sequencer.register_pipeline<toysequencer::TopOfBookCommand, toysequencer::TopOfBookEvent>(tob_adapter);

    sequencer.start();
    std::cout << "sequencer started, listening for commands on " << cmd_addr << ":" << cmd_port
              << " and publishing events to " << mcast_addr << ":" << events_port << std::endl;

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
