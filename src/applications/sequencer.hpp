#pragma once

#include "core/command_bus.hpp"
#include "core/sender_iface.hpp"
#include "generated/messages.pb.h"
#include <atomic>
#include <cstdint>

class Sequencer {
public:
  Sequencer(ISender &sender);
  void attach_command_bus(CommandBus &bus);

  // accept a TextCommand -> assigns seq, converts to TextEvent and multicasts
  uint64_t publish(const toysequencer::TextCommand &command);

  // for retransmit requests
  void retransmit(uint64_t from_seq, uint64_t to_seq);

private:
  std::atomic<uint64_t> next_seq_{1}; // start at 1
  ISender &sender_;
  CommandBus *bus_{nullptr};
};
