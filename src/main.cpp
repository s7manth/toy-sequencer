#include "applications/ping.hpp"
#include "applications/pong.hpp"
#include "applications/receiver.hpp"
#include "applications/sequencer.hpp"
#include "core/fanout_sender.hpp"
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
    const uint16_t sequencer_port = 30001;
    const uint16_t ping_port = 30002;
    const uint16_t pong_port = 30003;
    const uint8_t mcast_ttl = 1;

    // Fanout sender to multicast to all ports
    FanoutSender fanout;
    auto seq_sender = std::make_shared<MulticastSender>(
        mcast_addr, sequencer_port, mcast_ttl);
    auto ping_sender =
        std::make_shared<MulticastSender>(mcast_addr, ping_port, mcast_ttl);
    auto pong_sender =
        std::make_shared<MulticastSender>(mcast_addr, pong_port, mcast_ttl);
    fanout.add_sender(seq_sender);
    fanout.add_sender(ping_sender);
    fanout.add_sender(pong_sender);

    Sequencer sequencer(fanout);
    CommandBus bus;
    sequencer.attach_command_bus(bus);
    sequencer.start();

    auto log = [](const std::string &s) { std::cout << s << std::endl; };

    PingApp ping(bus, log);
    PongApp pong(log);

    // Each application listens on its own port
    Receiver ping_rx(
        [&](const toysequencer::TextEvent &e) { ping.on_event(e); });
    Receiver pong_rx(
        [&](const toysequencer::TextEvent &e) { pong.on_event(e); });

    MulticastReceiver rx_ping(mcast_addr, ping_port);
    MulticastReceiver rx_pong(mcast_addr, pong_port);

    rx_ping.subscribe([&](const uint8_t *data, size_t len) {
      ping_rx.on_datagram(data, len);
    });
    rx_pong.subscribe([&](const uint8_t *data, size_t len) {
      pong_rx.on_datagram(data, len);
    });

    rx_ping.start();
    rx_pong.start();

    std::thread ping_thread([&] { ping.run(); });

    // let system run briefly
    if (ping_thread.joinable()) {
      ping_thread.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    sequencer.stop();

    rx_ping.stop();
    rx_pong.stop();

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
