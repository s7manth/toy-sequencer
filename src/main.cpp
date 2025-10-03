#include "applications/ping.hpp"
#include "applications/pong.hpp"
#include "applications/scrappy.hpp"
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
    const std::string mcast_addr = "239.255.0.1";
    const uint16_t events_port = 30001; // single events stream
    const uint8_t mcast_ttl = 1;

    MulticastSender events_sender(mcast_addr, events_port, mcast_ttl);

    Sequencer sequencer(events_sender);
    CommandBus bus;
    sequencer.attach_command_bus(bus);
    sequencer.start();

    auto log = [](const std::string &s) { std::cout << s << std::endl; };

    uint64_t ping_instance_id = sequencer.assign_instance_id();
    uint64_t pong_instance_id = sequencer.assign_instance_id();

    PingApp ping(bus, log, ping_instance_id, pong_instance_id);
    PongApp pong(bus, log, pong_instance_id, ping_instance_id);

    ScrappyApp scrappy("sequenced_events.txt");
    sequencer.subscribe_to_events(
        [&](const toysequencer::TextEvent &event) { scrappy.on_event(event); });

    MulticastReceiver rx_events(mcast_addr, events_port);

    rx_events.subscribe([&](const uint8_t *data, size_t len) {
      ping.on_datagram<toysequencer::TextEvent>(data, len);
    });
    rx_events.subscribe([&](const uint8_t *data, size_t len) {
      pong.on_datagram<toysequencer::TextEvent>(data, len);
    });

    rx_events.start();

    std::thread ping_thread([&] {
      toysequencer::TextCommand cmd;
      cmd.set_text("PING1");
      cmd.set_tin(pong_instance_id);
      ping.send_command(cmd, ping_instance_id);

      toysequencer::TextCommand cmd2;
      cmd2.set_text("PING2");
      cmd2.set_tin(pong_instance_id);
      ping.send_command(cmd2, ping_instance_id);
    });

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
