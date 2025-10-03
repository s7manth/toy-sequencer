#include "../core/command_bus.hpp"
#include "../core/multicast_receiver.hpp"
#include "../generated/messages.pb.h"
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

    CommandBus bus;
    auto log = [](const std::string &s) { std::cout << s << std::endl; };

    const std::string mcast_addr = "239.255.0.1";
    const uint16_t events_port = 30001;

    // Assign instance ids locally for filtering
    uint64_t pong_instance_id = 2; // can be passed via CLI later
    uint64_t ping_instance_id = 1;

    PongApp pong(bus, log, pong_instance_id, ping_instance_id);

    MulticastReceiver rx_events(mcast_addr, events_port);
    rx_events.subscribe([&](const uint8_t *data, size_t len) {
      pong.on_datagram<toysequencer::TextEvent>(data, len);
    });
    rx_events.start();

    std::cout << "pong listening for TextEvent on " << mcast_addr << ":"
              << events_port << ", instance=" << pong_instance_id << std::endl;

    while (running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    rx_events.stop();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "pong error: " << e.what() << std::endl;
    return 1;
  }
}
