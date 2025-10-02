#include "sequencer.hpp"
#include <chrono>
#include <iostream>

Sequencer::Sequencer(ISender &sender) : sender_(sender) {}

void Sequencer::attach_command_bus(CommandBus &bus) {
  bus_ = &bus;
  bus.subscribe(
      [this](const toysequencer::TextCommand &cmd, uint64_t sender_id) {
        {
          std::lock_guard<std::mutex> lock(queue_mutex_);
          queue_.push({cmd, sender_id});
        }
        queue_cv_.notify_one();
      });
}

void Sequencer::start() {
  if (running_.exchange(true)) {
    return;
  }
  worker_ = std::thread([this] { this->worker_loop(); });
}

void Sequencer::stop() {
  if (!running_.exchange(false)) {
    return;
  }
  queue_cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
}

uint64_t Sequencer::publish(const toysequencer::TextCommand &command,
                            uint64_t sender_id) {
  uint64_t seq = next_seq_.fetch_add(1);

  toysequencer::TextEvent event;
  event.set_seq(seq);
  event.set_text(command.text());
  uint64_t ts =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  event.set_timestamp(ts);
  event.set_sid(sender_id);     // Set sender instance ID
  event.set_tin(command.tin()); // Copy TIN from command

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

uint64_t Sequencer::assign_instance_id() {
  return next_instance_id_.fetch_add(1);
}

void Sequencer::retransmit(uint64_t from_seq, uint64_t to_seq) {
  // TODO: implement retransmission logic
  // this would typically involve:
  // 1. looking up messages in the persistent log (WAL)
  // 2. resending them via multicast
  std::cout << "Retransmit requested for sequences " << from_seq << " to "
            << to_seq << std::endl;
}

void Sequencer::worker_loop() {
  while (running_) {
    std::pair<toysequencer::TextCommand, uint64_t> cmd_pair;
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      queue_cv_.wait(lock, [this] { return !running_ || !queue_.empty(); });
      if (!running_ && queue_.empty()) {
        return;
      }
      cmd_pair = queue_.front();
      queue_.pop();
    }
    this->publish(cmd_pair.first, cmd_pair.second);
  }
}
