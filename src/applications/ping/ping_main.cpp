#include "../core/command_bus.hpp"
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

    CommandBus bus;
    auto log = [](const std::string &s) { std::cout << s << std::endl; };

    const std::string mcast_addr = "239.255.0.1";
    const uint16_t events_port = 30001;

    uint64_t ping_instance_id = 1;
    uint64_t pong_instance_id = 2;

    PingApp ping(mcast_addr, events_port, 1, bus, log, ping_instance_id, pong_instance_id);
    ping.start();
    std::cout << "ping listening for TextEvent on " << mcast_addr << ":"
              << events_port << ", instance=" << ping_instance_id << std::endl;

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
