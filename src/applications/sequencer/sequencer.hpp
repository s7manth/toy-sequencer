#pragma once

#include "../adapters.hpp"
#include "../application.hpp"
#include "core/command_receiver.hpp"
#include "core/event_sender.hpp"
#include "generated/messages.pb.h"
#include "utils/instanceid_utils.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

class SequencerT : public Application, public IEventSender<SequencerT>, public CommandReceiver<SequencerT> {
public:
  SequencerT(const std::string &cmd_multicast_address, const uint16_t cmd_port,
             const std::string &events_multicast_address, const uint16_t events_port, const uint8_t ttl)
      : IEventSender<SequencerT>(events_multicast_address, events_port, ttl),
        CommandReceiver<SequencerT>(cmd_multicast_address, cmd_port) {}

  virtual ~SequencerT() = default;

  void on_command(const toysequencer::TextCommand &cmd) {
    std::cout << "Sequencer received TextCommand: " << cmd.DebugString() << std::endl;

    uint64_t seq = next_seq_.fetch_add(1);
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::high_resolution_clock::now().time_since_epoch())
                      .count();

    this->send(text_adapter.make_event(cmd, seq, 8888, ts));
  }

  void on_command(const toysequencer::TopOfBookCommand &cmd) {
    std::cout << "Sequencer received TopOfBookCommand: " << cmd.DebugString() << std::endl;

    uint64_t seq = next_seq_.fetch_add(1);
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::high_resolution_clock::now().time_since_epoch())
                      .count();

    this->send(tob_adapter.make_event(cmd, seq, 9999, ts));
  }

  template <typename EventT> void send_event(const EventT &event) {
    std::vector<uint8_t> data = serialize_event(event);
    this->send_m(data);
  }

  void start() override { CommandReceiver<SequencerT>::start(); }

  void stop() override { CommandReceiver<SequencerT>::stop(); }

  uint64_t get_instance_id() const override { return InstanceIdUtils::get_instance_id("SEQ"); }

private:
  template <typename EventT> std::vector<uint8_t> serialize_event(const EventT &event) {
    std::vector<uint8_t> payload(event.ByteSizeLong());
    event.SerializeToArray(payload.data(), static_cast<int>(payload.size()));
    return payload;
  }

  std::atomic<uint64_t> next_seq_{1}; // start at 1

  adapters::TextCommandToTextEvent text_adapter;
  adapters::TopOfBookCommandToTopOfBookEvent tob_adapter;
};

using Sequencer = SequencerT;
