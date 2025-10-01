#include "../stubs/stub_multicast_sender.hpp"
#include "../support/test_harness.hpp"
#include "applications/receiver.hpp"
#include "applications/sequencer.hpp"
#include "msg/message.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
  TestHarness harness;
  auto &stubSender = harness.get_stub_sender();

  Sequencer seq(stubSender);

  std::vector<Message> received;
  Receiver rx([&](const Message &m) { received.push_back(m); });

  // Scenarios adapted from test_multicast.cpp
  std::vector<std::string> test_messages = {
      "Hello, multicast world!", "This is message #2", "Testing UDP multicast",
      "Final test message"};

  // Publish messages via Sequencer
  std::vector<uint64_t> seqNos;
  for (const auto &text : test_messages) {
    std::vector<uint8_t> payload(text.begin(), text.end());
    uint64_t s = seq.publish(1, payload);
    seqNos.push_back(s);
  }

  // Deliver what was "sent" through the stub to our receiver
  for (const auto &datagram : stubSender.get_sent_datagrams()) {
    rx.on_datagram(datagram.data(), datagram.size());
  }

  // Assertions: we received all, in order, with correct metadata
  assert(received.size() == test_messages.size());
  for (size_t i = 0; i < received.size(); ++i) {
    const auto &msg = received[i];
    assert(msg.header.seq == seqNos[i]);
    assert(msg.header.type == 1);
    assert(msg.header.payload_size == test_messages[i].size());
    std::string got(msg.payload.begin(), msg.payload.end());
    assert(got == test_messages[i]);
  }

  std::cout << "test_sequencer_basic passed" << std::endl;
  return 0;
}
