#include "../stubs/stub_multicast_sender.hpp"
#include "../support/test_harness.hpp"
#include "applications/ping.hpp"
#include "applications/pong.hpp"
#include "applications/receiver.hpp"
#include "applications/sequencer.hpp"
#include "core/command_bus.hpp"
#include "generated/messages.pb.h"
#include <cassert>
#include <iostream>

int main() {
  std::cout << "=== Test: Ping and Pong Application Integration ==="
            << std::endl;

  TestHarness harness;
  auto &stubSender = harness.get_stub_sender();
  Sequencer seq(stubSender);
  CommandBus bus;
  seq.attach_command_bus(bus);

  // Assign instance IDs
  uint64_t ping_id = seq.assign_instance_id();
  uint64_t pong_id = seq.assign_instance_id();

  // Create applications
  std::vector<std::string> ping_logs, pong_logs;
  auto ping_logger = [&](const std::string &msg) { ping_logs.push_back(msg); };
  auto pong_logger = [&](const std::string &msg) { pong_logs.push_back(msg); };

  PingApp ping(bus, ping_logger, ping_id);
  PongApp pong(bus, pong_logger, pong_id);

  // Create receivers
  std::vector<toysequencer::TextEvent> ping_received, pong_received;
  Receiver ping_rx(
      [&](const toysequencer::TextEvent &e) { ping_received.push_back(e); },
      ping_id);
  Receiver pong_rx(
      [&](const toysequencer::TextEvent &e) { pong_received.push_back(e); },
      pong_id);

  // Send commands using the applications
  toysequencer::TextCommand ping_cmd;
  ping_cmd.set_text("PING_MESSAGE");
  ping_cmd.set_tin(ping_id);
  ping.send_command(ping_cmd, ping_id);

  toysequencer::TextCommand pong_cmd;
  pong_cmd.set_text("PONG_MESSAGE");
  pong_cmd.set_tin(pong_id);
  pong.send_command(pong_cmd, pong_id);

  // Process all datagrams
  for (const auto &datagram : stubSender.get_sent_datagrams()) {
    ping_rx.on_datagram(datagram.data(), datagram.size());
    pong_rx.on_datagram(datagram.data(), datagram.size());
  }

  // Verify applications work correctly
  assert(ping_received.size() == 1);
  assert(pong_received.size() == 1);
  assert(ping_received[0].text() == "PING_MESSAGE");
  assert(pong_received[0].text() == "PONG_MESSAGE");

  std::cout << "✓ Ping application sent and received message correctly"
            << std::endl;
  std::cout << "✓ Pong application sent and received message correctly"
            << std::endl;
  std::cout << "✓ Test PASSED" << std::endl;
  return 0;
}
