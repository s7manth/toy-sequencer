#include "../stubs/stub_multicast_sender.hpp"
#include "../support/test_harness.hpp"
#include "applications/sequencer/sequencer.hpp"
#include "core/command_bus.hpp"
#include "core/event_receiver.hpp"
#include "generated/messages.pb.h"
#include <cassert>
#include <iostream>

int main() {
  std::cout << "=== Test: TIN Filtering ===" << std::endl;

  TestHarness harness;
  auto &stubSender = harness.get_stub_sender();
  Sequencer seq(stubSender);
  CommandBus bus;
  seq.attach_command_bus(bus);

  uint64_t sender_id = seq.assign_instance_id();
  uint64_t target_id = seq.assign_instance_id();
  uint64_t other_id = seq.assign_instance_id();

  // Send command targeting specific instance
  toysequencer::TextCommand cmd;
  cmd.set_text("TARGETED_MESSAGE");
  cmd.set_tin(target_id);
  bus.publish(cmd, sender_id);

  // Create receivers for both target and non-target
  std::vector<toysequencer::TextEvent> target_received, other_received;
  struct TestReceiver : public EventReceiver<TestReceiver> {
    using EventReceiver::EventReceiver;
    std::function<void(const toysequencer::TextEvent &)> on;
    void on_event(const toysequencer::TextEvent &e) {
      if (on)
        on(e);
    }
  } target_rx(target_id), other_rx(other_id);
  target_rx.on = [&](const toysequencer::TextEvent &e) {
    target_received.push_back(e);
  };
  other_rx.on = [&](const toysequencer::TextEvent &e) {
    other_received.push_back(e);
  };

  // Process the sent datagram through both receivers
  for (const auto &datagram : stubSender.get_sent_datagrams()) {
    target_rx.on_datagram<toysequencer::TextEvent>(datagram.data(),
                                                   datagram.size());
    other_rx.on_datagram<toysequencer::TextEvent>(datagram.data(),
                                                  datagram.size());
  }

  // Verify TIN filtering worked
  assert(target_received.size() == 1);
  assert(other_received.size() == 0);
  assert(target_received[0].text() == "TARGETED_MESSAGE");

  std::cout << "✓ Target instance received " << target_received.size()
            << " events (expected 1)" << std::endl;
  std::cout << "✓ Other instance received " << other_received.size()
            << " events (expected 0)" << std::endl;
  std::cout << "✓ Test PASSED" << std::endl;
  return 0;
}
