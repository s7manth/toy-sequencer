#include "../stubs/stub_multicast_sender.hpp"
#include "../support/test_harness.hpp"
#include "applications/receiver.hpp"
#include "applications/sequencer.hpp"
#include "core/command_bus.hpp"
#include "generated/messages.pb.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
  TestHarness harness;
  auto &stubSender = harness.get_stub_sender();

  Sequencer seq(stubSender);
  CommandBus bus;
  seq.attach_command_bus(bus);

  std::vector<toysequencer::TextEvent> received;
  Receiver rx([&](const toysequencer::TextEvent &e) { received.push_back(e); });

  // Scenarios adapted from test_multicast.cpp
  std::vector<std::string> test_messages = {
      "Hello, multicast world!", "This is message #2", "Testing UDP multicast",
      "Final test message"};

  // Publish commands via CommandBus; Sequencer will convert to events and send
  std::vector<uint64_t> seqNos;
  for (const auto &text : test_messages) {
    toysequencer::TextCommand cmd;
    cmd.set_text(text);
    auto seqNo = seq.publish(cmd);
    seqNos.push_back(seqNo);
  }

  // Deliver what was "sent" through the stub to our receiver
  for (const auto &datagram : stubSender.get_sent_datagrams()) {
    rx.on_datagram(datagram.data(), datagram.size());
  }

  // Assertions: we received all, in order, with correct metadata
  assert(received.size() == test_messages.size());
  for (size_t i = 0; i < received.size(); ++i) {
    assert(ev.seq() == seqNos[i]);
    assert(ev.text() == test_messages[i]);
    assert(ev.timestamp() > 0);
  }

  std::cout << "test_sequencer_basic passed" << std::endl;
  return 0;
}
