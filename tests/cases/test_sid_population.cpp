#include "../stubs/stub_multicast_sender.hpp"
#include "../support/test_harness.hpp"
#include "applications/adapters.hpp"
#include "applications/sequencer/sequencer.hpp"
#include "core/command_bus.hpp"
#include "core/event_receiver.hpp"
#include "generated/messages.pb.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  std::cout << "=== Test: SID Population in Events ===" << std::endl;

  TestHarness harness;
  auto &stubSender = harness.get_stub_sender();
  Sequencer seq(stubSender);
  CommandBus bus;
  seq.attach_command_bus(bus);
  adapters::TextCommandToTextEvent adapter;
  seq.register_pipeline<toysequencer::TextCommand, toysequencer::TextEvent>(
      adapter);

  uint64_t sender_id = seq.assign_instance_id();
  uint64_t target_id = seq.assign_instance_id();

  // Start the sequencer to process commands
  seq.start();

  // Send command with specific sender ID
  toysequencer::TextCommand cmd;
  cmd.set_text("TEST_MESSAGE");
  cmd.set_tin(target_id);
  bus.publish(cmd, sender_id);

  // Give the sequencer time to process the command
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Create receiver for target
  std::vector<toysequencer::TextEvent> received;
  struct TestReceiver : public EventReceiver<TestReceiver> {
    using EventReceiver::EventReceiver;
    std::function<void(const toysequencer::TextEvent &)> on;
    void on_event(const toysequencer::TextEvent &e) {
      if (on)
        on(e);
    }
  } rx(target_id);
  rx.on = [&](const toysequencer::TextEvent &e) { received.push_back(e); };

  // Process the sent datagram
  for (const auto &datagram : stubSender.get_sent_datagrams()) {
    rx.on_datagram<toysequencer::TextEvent>(datagram.data(), datagram.size());
  }

  // Verify SID is correctly set
  assert(received.size() == 1);
  assert(received[0].sid() == sender_id);
  assert(received[0].text() == "TEST_MESSAGE");

  std::cout << "✓ Event has correct SID=" << received[0].sid() << " (expected "
            << sender_id << ")" << std::endl;
  std::cout << "✓ Test PASSED" << std::endl;

  // Clean up
  seq.stop();

  return 0;
}
