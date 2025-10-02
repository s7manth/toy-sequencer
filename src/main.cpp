#include "applications/ping.hpp"
#include "applications/pong.hpp"
#include "applications/receiver.hpp"
#include "applications/sequencer.hpp"
#include "core/multicast_receiver.hpp"
#include "core/multicast_sender.hpp"
#include "generated/messages.pb.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int, char **) {
  try {
    // Configure multicast transport
    const std::string mcast_addr = "239.255.0.1";
    const uint16_t events_port = 30001; // single events stream
    const uint8_t mcast_ttl = 1;

    // Single events sender/port
    MulticastSender events_sender(mcast_addr, events_port, mcast_ttl);

    Sequencer sequencer(events_sender);
    CommandBus bus;
    sequencer.attach_command_bus(bus);
    sequencer.start();

    auto log = [](const std::string &s) { std::cout << s << std::endl; };

    // Assign instance IDs to applications
    uint64_t ping_instance_id = sequencer.assign_instance_id();
    uint64_t pong_instance_id = sequencer.assign_instance_id();

    PingApp ping(bus, log, ping_instance_id);
    PongApp pong(bus, log, pong_instance_id);

    // Both applications subscribe to the single events stream
    Receiver ping_rx(
        [&](const toysequencer::TextEvent &e) { ping.on_event(e); },
        ping_instance_id);
    Receiver pong_rx(
        [&](const toysequencer::TextEvent &e) { pong.on_event(e); },
        pong_instance_id);

    MulticastReceiver rx_events(mcast_addr, events_port);

    rx_events.subscribe([&](const uint8_t *data, size_t len) {
      ping_rx.on_datagram(data, len);
    });
    rx_events.subscribe([&](const uint8_t *data, size_t len) {
      pong_rx.on_datagram(data, len);
    });

    rx_events.start();

    std::thread ping_thread([&] { ping.run(); });

    // let system run briefly
    if (ping_thread.joinable()) {
      ping_thread.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    sequencer.stop();

    rx_events.stop();

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
