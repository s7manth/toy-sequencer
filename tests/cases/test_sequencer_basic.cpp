#include "../stubs/stub_multicast_sender.hpp"
#include "../support/test_harness.hpp"
#include "applications/sequencer.hpp"
#include "core/command_bus.hpp"
#include "core/event_receiver.hpp"
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
  struct TestReceiver : public EventReceiver<TestReceiver> {
    using EventReceiver::EventReceiver;
    std::function<void(const toysequencer::TextEvent &)> on;
    void on_event(const toysequencer::TextEvent &e) {
      if (on)
        on(e);
    }
  } rx(1);
  rx.on = [&](const toysequencer::TextEvent &e) { received.push_back(e); };

  // Scenarios adapted from test_multicast.cpp
  std::vector<std::string> test_messages = {
      "Hello, multicast world!", "This is message #2", "Testing UDP multicast",
      "Final test message"};

  // Publish commands via CommandBus; Sequencer will convert to events and send
  std::vector<uint64_t> seqNos;
  for (const auto &text : test_messages) {
    toysequencer::TextCommand cmd;
    cmd.set_text(text);
    cmd.set_tin(1);      // Set target instance ID
    bus.publish(cmd, 1); // Publish with sender ID 1
  }

  // Deliver what was "sent" through the stub to our receiver
  for (const auto &datagram : stubSender.get_sent_datagrams()) {
    rx.on_datagram<toysequencer::TextEvent>(datagram.data(), datagram.size());
  }

  // Assertions: we received all, in order, with correct metadata
  assert(received.size() == test_messages.size());
  for (size_t i = 0; i < received.size(); ++i) {
    assert(received[i].seq() == i + 1); // Sequence numbers start at 1
    assert(received[i].text() == test_messages[i]);
    assert(received[i].timestamp() > 0);
    assert(received[i].sid() == 1); // Sender ID should be 1
    assert(received[i].tin() == 1); // Target ID should be 1
  }

  std::cout << "test_sequencer_basic passed" << std::endl;
  return 0;
}
