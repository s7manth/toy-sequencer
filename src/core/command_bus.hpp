#pragma once

#include "generated/messages.pb.h"
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

// Simple in-process command bus. Applications publish commands here.
// Sequencer subscribes and emits events to the multicast stream.
class CommandBus {
public:
  using CommandHandler = std::function<void(const toysequencer::TextCommand &,
                                            uint64_t sender_id)>;

  void subscribe(CommandHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.push_back(std::move(handler));
  }

  void publish(const toysequencer::TextCommand &command,
               uint64_t sender_id) const {
    // Call without holding lock to avoid handler reentrancy issues.
    std::vector<CommandHandler> copy;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      copy = handlers_;
    }
    for (auto &h : copy) {
      h(command, sender_id);
    }
  }

private:
  mutable std::mutex mutex_;
  std::vector<CommandHandler> handlers_;
};
