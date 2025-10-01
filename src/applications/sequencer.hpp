#pragma once

#include "core/multicast_sender.hpp"
#include <atomic>
#include <cstdint>

class Sequencer {
public:
  Sequencer(MulticastSender &sender);

  // accept an application event (payload & type) -> assigns seq and multicasts
  uint64_t publish(uint32_t type, const std::vector<uint8_t> &payload);

  // for retransmit requests
  void retransmit(uint64_t from_seq, uint64_t to_seq);

private:
  std::atomic<uint64_t> next_seq_{1}; // start at 1
  MulticastSender &sender_;
};
