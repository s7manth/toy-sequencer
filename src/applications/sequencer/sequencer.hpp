#pragma once

#include "core/command_bus.hpp"
#include "core/command_receiver.hpp"
#include "core/event_sender.hpp"
#include "generated/messages.pb.h"
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

class SequencerT : public IEventSender<SequencerT>, public CommandReceiver<SequencerT> {
public:
  SequencerT(const std::string &cmd_multicast_address, uint16_t cmd_port, const std::string &events_multicast_address,
             uint16_t events_port, uint8_t ttl)
      : IEventSender<SequencerT>(events_multicast_address, events_port, ttl),
        CommandReceiver<SequencerT>(cmd_multicast_address, cmd_port) {}

  virtual ~SequencerT() = default;

  void attach_command_bus(CommandBus &bus) { bus_ = &bus; }

  void on_command(const toysequencer::TextCommand &cmd) {
    if (!bus_) {
      return;
    }
    std::cout << "Sequencer received TextCommand: " << cmd.DebugString() << std::endl;
    bus_->publish(cmd, cmd.tin());
  }

  void on_command(const toysequencer::TopOfBookCommand &cmd) {
    if (!bus_) {
      return;
    }
    std::cout << "Sequencer received TopOfBookCommand: " << cmd.DebugString() << std::endl;
    bus_->publish(cmd, cmd.tin());
  }

  template <typename CommandT, typename EventT, typename AdapterT> void register_pipeline(AdapterT adapter) {
    if (!bus_) {
      return;
    }
    bus_->template subscribe<CommandT>([this, adapter](const CommandT &cmd, uint64_t sender_id) {
      std::cout << "Sequencer processing command from sender_id=" << sender_id << ", cmd=" << cmd.DebugString()
                << std::endl;
      {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push([this, adapter, cmd, sender_id]() {
          uint64_t seq = next_seq_.fetch_add(1);
          uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::high_resolution_clock::now().time_since_epoch())
                            .count();
          EventT event = adapter.make_event(cmd, seq, sender_id, ts);
          std::cout << "Sequencer processing event seq=" << seq << ", sender_id=" << sender_id
                    << ", event=" << event.DebugString() << std::endl;
          send_event(event);
          notify_event_subscribers<EventT>(event);
        });
      }
      queue_cv_.notify_one();
    });
  }

  template <typename EventT> void send_event(const EventT &event) {
    std::vector<uint8_t> payload = serialize_event(event);
    bool success = IEventSender<SequencerT>::send_m(payload);
    if (!success) {
      std::cerr << "Failed to send event with sequence " << event.seq() << std::endl;
    } else {
      std::cout << "Successfully sent event seq=" << event.seq() << std::endl;
    }
  }

  void start() {
    if (running_.exchange(true)) {
      return;
    }
    worker_ = std::thread([this] { this->worker_loop(); });
    CommandReceiver<SequencerT>::start();
  }

  void stop() {
    if (!running_.exchange(false)) {
      return;
    }
    queue_cv_.notify_all();
    if (worker_.joinable()) {
      worker_.join();
    }
    CommandReceiver<SequencerT>::stop();
  }

  void retransmit(uint64_t /*from_seq*/, uint64_t /*to_seq*/) {}

  template <typename EventT> void subscribe_to_events(std::function<void(const EventT &)> handler) {
    std::lock_guard<std::mutex> lock(event_handlers_mutex_);
    auto &bucket = event_handlers_[std::type_index(typeid(EventT))];
    bucket.push_back([h = std::move(handler)](const void *ptr) { h(*static_cast<const EventT *>(ptr)); });
  }

private:
  template <typename EventT> std::vector<uint8_t> serialize_event(const EventT &event) {
    std::vector<uint8_t> payload(event.ByteSizeLong());
    event.SerializeToArray(payload.data(), static_cast<int>(payload.size()));
    return payload;
  }

  void worker_loop() {
    while (running_) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this] { return !running_ || !task_queue_.empty(); });
        if (!running_ && task_queue_.empty()) {
          return;
        }
        task = std::move(task_queue_.front());
        task_queue_.pop();
      }
      task();
    }
  }

  template <typename EventT> void notify_event_subscribers(const EventT &event) {
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

  std::atomic<uint64_t> next_seq_{1}; // start at 1
  CommandBus *bus_{nullptr};

  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::queue<std::function<void()>> task_queue_;
  std::thread worker_;
  std::atomic<bool> running_{false};

  std::mutex event_handlers_mutex_;
  std::unordered_map<std::type_index, std::vector<std::function<void(const void *)>>> event_handlers_;
};

using Sequencer = SequencerT;
