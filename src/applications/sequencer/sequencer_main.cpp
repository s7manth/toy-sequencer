#include "../adapters.hpp"
#include "../core/command_bus.hpp"
#include "../core/multicast_receiver.hpp"
#include "../core/multicast_sender.hpp"
#include "../generated/messages.pb.h"
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

    MulticastSender events_sender(mcast_addr, events_port, mcast_ttl);

    Sequencer sequencer(events_sender);
    CommandBus bus;
    sequencer.attach_command_bus(bus);

    adapters::TextCommandToTextEvent text_adapter;
    adapters::TopOfBookCommandToTopOfBookEvent tob_adapter;
    sequencer
        .register_pipeline<toysequencer::TextCommand, toysequencer::TextEvent>(
            text_adapter);
    sequencer.register_pipeline<toysequencer::TopOfBookCommand,
                                toysequencer::TopOfBookEvent>(tob_adapter);

    MulticastReceiver cmd_rx(cmd_addr, cmd_port);
    cmd_rx.subscribe([&](const uint8_t *data, size_t len) {
      std::cout << "Received " << len << " bytes on command channel"
                << std::endl;

      {
        toysequencer::TopOfBookCommand tob;
        if (tob.ParseFromArray(data, static_cast<int>(len))) {
          if (!tob.symbol().empty() && tob.tin() > 0) {
            std::cout << "Parsed TopOfBookCommand: " << tob.DebugString()
                      << std::endl;
            bus.publish(tob, tob.tin());
            return;
          }
        }
      }

      {
        toysequencer::TextCommand tc;
        if (tc.ParseFromArray(data, static_cast<int>(len))) {
          if (!tc.text().empty() && tc.tin() > 0) {
            std::cout << "Parsed TextCommand: " << tc.DebugString()
                      << std::endl;
            bus.publish(tc, tc.tin());
            return;
          }
        }
      }

      std::cout << "Failed to parse received data as any known command type"
                << std::endl;
    });

    sequencer.start();
    cmd_rx.start();
    std::cout << "sequencer started, publishing events to " << mcast_addr << ":"
              << events_port << std::endl;

    while (running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    cmd_rx.stop();
    sequencer.stop();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "sequencer error: " << e.what() << std::endl;
    return 1;
  }
}
