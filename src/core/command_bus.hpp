#pragma once

#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

class CommandBus {
public:
  template <typename CommandT>
  using CommandHandler =
      std::function<void(const CommandT &, uint64_t sender_id)>;

  template <typename CommandT>
  void subscribe(CommandHandler<CommandT> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto &bucket = handlers_[std::type_index(typeid(CommandT))];
    bucket.push_back([h = std::move(handler)](const void *ptr, uint64_t sid) {
      h(*static_cast<const CommandT *>(ptr), sid);
    });
  }

  template <typename CommandT>
  void publish(const CommandT &command, uint64_t sender_id) const {
    // call without holding lock to avoid handler reentrancy issues
    std::vector<std::function<void(const void *, uint64_t)>> copy;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = handlers_.find(std::type_index(typeid(CommandT)));
      if (it != handlers_.end()) {
        copy = it->second;
      }
    }
    for (auto &erased : copy) {
      erased(&command, sender_id);
    }
  }

private:
  using ErasedHandler = std::function<void(const void *, uint64_t)>;
  mutable std::mutex mutex_;
  mutable std::unordered_map<std::type_index, std::vector<ErasedHandler>>
      handlers_;
};
