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
  using CommandHandler = std::function<void(const toysequencer::TextCommand &)>;

  void subscribe(CommandHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.push_back(std::move(handler));
  }

  void publish(const toysequencer::TextCommand &command) const {
    // Call without holding lock to avoid handler reentrancy issues.
    std::vector<CommandHandler> copy;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      copy = handlers_;
    }
    for (auto &h : copy) {
      h(command);
    }
  }

private:
  mutable std::mutex mutex_;
  std::vector<CommandHandler> handlers_;
};
