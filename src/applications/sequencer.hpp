#pragma once

#include "core/command_bus.hpp"
#include "core/sender_iface.hpp"
#include "generated/messages.pb.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>

class Sequencer {
public:
  Sequencer(ISender &sender);
  void attach_command_bus(CommandBus &bus);

  // start/stop background worker thread that sequences commands
  void start();
  void stop();

  // accept a TextCommand -> assigns seq, converts to TextEvent and multicasts
  uint64_t publish(const toysequencer::TextCommand &command,
                   uint64_t sender_id);

  // assign a unique instance ID to an application
  uint64_t assign_instance_id();

  // for retransmit requests
  void retransmit(uint64_t from_seq, uint64_t to_seq);

private:
  void worker_loop();

  std::atomic<uint64_t> next_seq_{1};         // start at 1
  std::atomic<uint64_t> next_instance_id_{1}; // start at 1
  ISender &sender_;
  CommandBus *bus_{nullptr};

  // background worker state
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::queue<std::pair<toysequencer::TextCommand, uint64_t>> queue_;
  std::thread worker_;
  std::atomic<bool> running_{false};
};
