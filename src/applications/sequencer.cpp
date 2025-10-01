#include "sequencer.hpp"
#include <chrono>
#include <iostream>

Sequencer::Sequencer(ISender &sender) : sender_(sender) {}

void Sequencer::attach_command_bus(CommandBus &bus) {
  bus_ = &bus;
  bus.subscribe(
      [this](const toysequencer::TextCommand &cmd) { this->publish(cmd); });
}

uint64_t Sequencer::publish(const toysequencer::TextCommand &command) {
  uint64_t seq = next_seq_.fetch_add(1);

  toysequencer::TextEvent event;
  event.set_seq(seq);
  event.set_text(command.text());
  uint64_t ts =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  event.set_timestamp(ts);

  std::string bytes;
  bytes.resize(event.ByteSizeLong());
  event.SerializeToArray(bytes.data(), static_cast<int>(bytes.size()));
  std::vector<uint8_t> payload(bytes.begin(), bytes.end());

  bool success = sender_.send(payload);

  if (!success) {
    std::cerr << "Failed to send message with sequence " << seq << std::endl;
  }

  return seq;
}

void Sequencer::retransmit(uint64_t from_seq, uint64_t to_seq) {
  // TODO: implement retransmission logic
  // this would typically involve:
  // 1. looking up messages in the persistent log (WAL)
  // 2. resending them via multicast
  std::cout << "Retransmit requested for sequences " << from_seq << " to "
            << to_seq << std::endl;
}
