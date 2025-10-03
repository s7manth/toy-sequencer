#pragma once

#include "core/command_bus.hpp"
#include "core/sender_iface.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <vector>

class SequencerT {
public:
  explicit SequencerT(ISender &sender) : sender_(sender) {}

  void attach_command_bus(CommandBus &bus) { bus_ = &bus; }

  template <typename CommandT, typename EventT, typename AdapterT>
  void register_pipeline(AdapterT adapter) {
    if (!bus_) {
      return;
    }
    bus_->template subscribe<CommandT>([this, adapter](const CommandT &cmd,
                                                       uint64_t sender_id) {
      {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push([this, adapter, cmd, sender_id]() {
          uint64_t seq = next_seq_.fetch_add(1);
          uint64_t ts =
              std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::high_resolution_clock::now().time_since_epoch())
                  .count();
          EventT event = adapter.make_event(cmd, seq, sender_id, ts);
          std::vector<uint8_t> payload = adapter.serialize(event);
          bool success = sender_.send(payload);
          if (!success) {
            std::cerr << "Failed to send message with sequence " << seq
                      << std::endl;
          }
          notify_event_subscribers<EventT>(event);
        });
      }
      queue_cv_.notify_one();
    });
  }

  // start/stop background worker thread that sequences commands
  void start() {
    if (running_.exchange(true)) {
      return;
    }
    worker_ = std::thread([this] { this->worker_loop(); });
  }
  void stop() {
    if (!running_.exchange(false)) {
      return;
    }
    queue_cv_.notify_all();
    if (worker_.joinable()) {
      worker_.join();
    }
  }

  uint64_t assign_instance_id() { return next_instance_id_.fetch_add(1); }

  void retransmit(uint64_t /*from_seq*/, uint64_t /*to_seq*/) {}

  template <typename EventT>
  void subscribe_to_events(std::function<void(const EventT &)> handler) {
    std::lock_guard<std::mutex> lock(event_handlers_mutex_);
    auto &bucket = event_handlers_[std::type_index(typeid(EventT))];
    bucket.push_back([h = std::move(handler)](const void *ptr) {
      h(*static_cast<const EventT *>(ptr));
    });
  }

private:
  void worker_loop() {
    while (running_) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock,
                       [this] { return !running_ || !task_queue_.empty(); });
        if (!running_ && task_queue_.empty()) {
          return;
        }
        task = std::move(task_queue_.front());
        task_queue_.pop();
      }
      task();
    }
  }

  template <typename EventT>
  void notify_event_subscribers(const EventT &event) {
    std::vector<std::function<void(const void *)>> copy;
    {
      std::lock_guard<std::mutex> lock(event_handlers_mutex_);
      auto it = event_handlers_.find(std::type_index(typeid(EventT)));
      if (it != event_handlers_.end()) {
        copy = it->second;
      }
    }
    for (auto &h : copy) {
      h(&event);
    }
  }

  std::atomic<uint64_t> next_seq_{1};         // start at 1
  std::atomic<uint64_t> next_instance_id_{1}; // start at 1
  ISender &sender_;
  CommandBus *bus_{nullptr};

  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::queue<std::function<void()>> task_queue_;
  std::thread worker_;
  std::atomic<bool> running_{false};

  // event subscription mechanism
  std::mutex event_handlers_mutex_;
  std::unordered_map<std::type_index,
                     std::vector<std::function<void(const void *)>>>
      event_handlers_;
};

using Sequencer = SequencerT;
