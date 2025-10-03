#include "../stubs/stub_multicast_sender.hpp"
#include "../support/test_harness.hpp"
#include "applications/ping.hpp"
#include "applications/pong.hpp"
#include "applications/sequencer.hpp"
#include "core/command_bus.hpp"
#include "core/event_receiver.hpp"
#include "generated/messages.pb.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

void test_cross_instance_communication() {
  std::cout << "=== Cross-Instance Communication ===" << std::endl;

  TestHarness harness;
  auto &stubSender = harness.get_stub_sender();
  Sequencer seq(stubSender);
  CommandBus bus;
  seq.attach_command_bus(bus);
  seq.start();

  uint64_t ping_id = seq.assign_instance_id();
  uint64_t pong_id = seq.assign_instance_id();
  uint64_t third_id = seq.assign_instance_id();

  // Create receivers for all instances
  std::vector<toysequencer::TextEvent> ping_received, pong_received,
      third_received;
  struct TestReceiver : public EventReceiver<TestReceiver> {
    using EventReceiver::EventReceiver;
    std::function<void(const toysequencer::TextEvent &)> on;
    void on_event(const toysequencer::TextEvent &e) {
      if (on)
        on(e);
    }
  } ping_rx(ping_id), pong_rx(pong_id), third_rx(third_id);
  ping_rx.on = [&](const toysequencer::TextEvent &e) {
    ping_received.push_back(e);
  };
  pong_rx.on = [&](const toysequencer::TextEvent &e) {
    pong_received.push_back(e);
  };
  third_rx.on = [&](const toysequencer::TextEvent &e) {
    third_received.push_back(e);
  };

  // Send commands: Ping -> Pong, Pong -> Third, Third -> Ping
  toysequencer::TextCommand cmd1;
  cmd1.set_text("PING_TO_PONG");
  cmd1.set_tin(pong_id);
  bus.publish(cmd1, ping_id);

  toysequencer::TextCommand cmd2;
  cmd2.set_text("PONG_TO_THIRD");
  cmd2.set_tin(third_id);
  bus.publish(cmd2, pong_id);

  toysequencer::TextCommand cmd3;
  cmd3.set_text("THIRD_TO_PING");
  cmd3.set_tin(ping_id);
  bus.publish(cmd3, third_id);

  // Give the sequencer worker thread time to process the commands
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  seq.stop();

  // Process all datagrams through all receivers
  for (const auto &datagram : stubSender.get_sent_datagrams()) {
    ping_rx.on_datagram<toysequencer::TextEvent>(datagram.data(),
                                                 datagram.size());
    pong_rx.on_datagram<toysequencer::TextEvent>(datagram.data(),
                                                 datagram.size());
    third_rx.on_datagram<toysequencer::TextEvent>(datagram.data(),
                                                  datagram.size());
  }

  // Verify each instance received only its targeted message
  assert(ping_received.size() == 1);
  assert(pong_received.size() == 1);
  assert(third_received.size() == 1);

  assert(ping_received[0].text() == "THIRD_TO_PING");
  assert(pong_received[0].text() == "PING_TO_PONG");
  assert(third_received[0].text() == "PONG_TO_THIRD");

  std::cout << "✓ Ping received: " << ping_received[0].text()
            << " (SID=" << ping_received[0].sid() << ")" << std::endl;
  std::cout << "✓ Pong received: " << pong_received[0].text()
            << " (SID=" << pong_received[0].sid() << ")" << std::endl;
  std::cout << "✓ Third received: " << third_received[0].text()
            << " (SID=" << third_received[0].sid() << ")" << std::endl;
  std::cout << "✓ Test 4 PASSED" << std::endl << std::endl;
}

void test_sequence_number_assignment() {
  std::cout << "=== Sequence Number Assignment ===" << std::endl;

  TestHarness harness;
  auto &stubSender = harness.get_stub_sender();
  Sequencer seq(stubSender);
  CommandBus bus;
  seq.attach_command_bus(bus);
  seq.start();

  uint64_t sender_id = seq.assign_instance_id();
  uint64_t target_id = seq.assign_instance_id();

  // Send multiple commands
  std::vector<std::string> messages = {"MSG1", "MSG2", "MSG3"};
  for (const auto &msg : messages) {
    toysequencer::TextCommand cmd;
    cmd.set_text(msg);
    cmd.set_tin(target_id);
    bus.publish(cmd, sender_id);
  }

  // Give the sequencer worker thread time to process the commands
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  seq.stop();

  // Create receiver
  std::vector<toysequencer::TextEvent> received;
  struct TestReceiver2 : public EventReceiver<TestReceiver2> {
    using EventReceiver::EventReceiver;
    std::function<void(const toysequencer::TextEvent &)> on;
    void on_event(const toysequencer::TextEvent &e) {
      if (on)
        on(e);
    }
  } rx(target_id);
  rx.on = [&](const toysequencer::TextEvent &e) { received.push_back(e); };

  // Process all datagrams
  for (const auto &datagram : stubSender.get_sent_datagrams()) {
    rx.on_datagram<toysequencer::TextEvent>(datagram.data(), datagram.size());
  }

  // Verify sequence numbers are consecutive starting from 1
  assert(received.size() == messages.size());
  for (size_t i = 0; i < received.size(); ++i) {
    assert(received[i].seq() == i + 1);
    assert(received[i].text() == messages[i]);
    std::cout << "✓ Event '" << received[i].text()
              << "' has seq=" << received[i].seq() << std::endl;
  }

  std::cout << "✓ Test 5 PASSED" << std::endl << std::endl;
}

void test_timestamp_assignment() {
  std::cout << "=== Timestamp Assignment ===" << std::endl;

  TestHarness harness;
  auto &stubSender = harness.get_stub_sender();
  Sequencer seq(stubSender);
  CommandBus bus;
  seq.attach_command_bus(bus);
  seq.start();

  uint64_t sender_id = seq.assign_instance_id();
  uint64_t target_id = seq.assign_instance_id();

  // Send a command
  toysequencer::TextCommand cmd;
  cmd.set_text("TIMESTAMP_TEST");
  cmd.set_tin(target_id);
  bus.publish(cmd, sender_id);

  // Give the sequencer worker thread time to process the commands
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  seq.stop();

  // Create receiver
  std::vector<toysequencer::TextEvent> received;
  struct TestReceiver3 : public EventReceiver<TestReceiver3> {
    using EventReceiver::EventReceiver;
    std::function<void(const toysequencer::TextEvent &)> on;
    void on_event(const toysequencer::TextEvent &e) {
      if (on)
        on(e);
    }
  } rx(target_id);
  rx.on = [&](const toysequencer::TextEvent &e) { received.push_back(e); };

  // Process the datagram
  for (const auto &datagram : stubSender.get_sent_datagrams()) {
    rx.on_datagram<toysequencer::TextEvent>(datagram.data(), datagram.size());
  }

  // Verify timestamp is set
  assert(received.size() == 1);
  assert(received[0].timestamp() > 0);

  std::cout << "✓ Event has timestamp=" << received[0].timestamp() << std::endl;
  std::cout << "✓ Test 6 PASSED" << std::endl << std::endl;
}

void test_ping_pong_integration() {
  std::cout << "=== Ping and Pong Application Integration ===" << std::endl;

  TestHarness harness;
  auto &stubSender = harness.get_stub_sender();
  Sequencer seq(stubSender);
  CommandBus bus;
  seq.attach_command_bus(bus);
  seq.start();

  // Assign instance IDs
  uint64_t ping_id = seq.assign_instance_id();
  uint64_t pong_id = seq.assign_instance_id();

  // Create applications
  std::vector<std::string> ping_logs, pong_logs;
  auto ping_logger = [&](const std::string &msg) { ping_logs.push_back(msg); };
  auto pong_logger = [&](const std::string &msg) { pong_logs.push_back(msg); };

  PingApp ping(bus, ping_logger, ping_id, pong_id);
  PongApp pong(bus, pong_logger, pong_id, ping_id);

  // Create receivers
  std::vector<toysequencer::TextEvent> ping_received, pong_received;
  struct TestReceiver4 : public EventReceiver<TestReceiver4> {
    using EventReceiver::EventReceiver;
    std::function<void(const toysequencer::TextEvent &)> on;
    void on_event(const toysequencer::TextEvent &e) {
      if (on)
        on(e);
    }
  } ping_rx(ping_id), pong_rx(pong_id);
  ping_rx.on = [&](const toysequencer::TextEvent &e) {
    ping_received.push_back(e);
  };
  pong_rx.on = [&](const toysequencer::TextEvent &e) {
    pong_received.push_back(e);
  };

  // Send commands using the applications
  toysequencer::TextCommand ping_cmd;
  ping_cmd.set_text("PING_MESSAGE");
  ping_cmd.set_tin(ping_id);
  ping.send_command(ping_cmd, ping_id);

  toysequencer::TextCommand pong_cmd;
  pong_cmd.set_text("PONG_MESSAGE");
  pong_cmd.set_tin(pong_id);
  pong.send_command(pong_cmd, pong_id);

  // Give the sequencer worker thread time to process the commands
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  seq.stop();

  // Process all datagrams
  for (const auto &datagram : stubSender.get_sent_datagrams()) {
    ping_rx.on_datagram<toysequencer::TextEvent>(datagram.data(),
                                                 datagram.size());
    pong_rx.on_datagram<toysequencer::TextEvent>(datagram.data(),
                                                 datagram.size());
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
  std::cout << "✓ Test 7 PASSED" << std::endl << std::endl;
}

int main() {
  std::cout << "🧪 Running SID/TIN Functionality Test Suite" << std::endl
            << std::endl;

  try {
    test_cross_instance_communication();
    test_sequence_number_assignment();
    test_timestamp_assignment();
    test_ping_pong_integration();

    std::cout << "🎉 All SID/TIN functionality tests PASSED!" << std::endl;
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "❌ Test failed with unknown exception" << std::endl;
    return 1;
  }
}