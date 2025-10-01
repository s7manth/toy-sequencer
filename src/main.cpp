#include "applications/ping_pong.hpp"
#include "applications/receiver.hpp"
#include "applications/sequencer.hpp"
#include "core/inproc_multicast.hpp"
#include "generated/messages.pb.h"
#include <chrono>
#include <iostream>
#include <thread>

int main(int, char **) {
  try {
    InprocMulticastSender inproc_sender;
    Sequencer sequencer(inproc_sender);
    CommandBus bus;
    sequencer.attach_command_bus(bus);
    sequencer.start();

    auto log = [](const std::string &s) { std::cout << s << std::endl; };

    PingApp ping(bus, log);
    PongApp pong(log);

    Receiver ping_rx(
        [&](const toysequencer::TextEvent &e) { ping.on_event(e); });
    Receiver pong_rx(
        [&](const toysequencer::TextEvent &e) { pong.on_event(e); });

    inproc_sender.register_subscriber(
        [&](const std::vector<uint8_t> &bytes) { ping_rx.on_bytes(bytes); });
    inproc_sender.register_subscriber(
        [&](const std::vector<uint8_t> &bytes) { pong_rx.on_bytes(bytes); });

    std::thread ping_thread([&] { ping.start(); });

    // let system run briefly
    if (ping_thread.joinable()) {
      ping_thread.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    sequencer.stop();

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
