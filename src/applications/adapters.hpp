#pragma once

#include "generated/messages.pb.h"
#include <cstdint>
#include <string>
#include <vector>

namespace adapters {

struct TextCommandToTextEvent {
  toysequencer::TextEvent make_event(const toysequencer::TextCommand &command, uint64_t seq,
                                     uint64_t sender_id, uint64_t ts) const {
    toysequencer::TextEvent event;
    event.set_seq(seq);
    event.set_text(command.text());
    event.set_timestamp(ts);
    event.set_sid(sender_id);
    event.set_tin(command.tin());
    return event;
  }

  std::vector<uint8_t> serialize(const toysequencer::TextEvent &event) const {
    std::string bytes;
    bytes.resize(event.ByteSizeLong());
    event.SerializeToArray(bytes.data(), static_cast<int>(bytes.size()));
    return std::vector<uint8_t>(bytes.begin(), bytes.end());
  }
};

} // namespace adapters

// Market data adapters live in the same header for simplicity
namespace adapters {

struct TopOfBookCommandToTopOfBookEvent {
  toysequencer::TopOfBookEvent make_event(const toysequencer::TopOfBookCommand &command,
                                          uint64_t seq, uint64_t sender_id, uint64_t ts) const {
    toysequencer::TopOfBookEvent event;
    event.set_seq(seq);
    event.set_timestamp(ts);
    event.set_sid(sender_id);
    event.set_tin(command.tin());

    event.set_symbol(command.symbol());
    event.set_bid_price(command.bid_price());
    event.set_bid_size(command.bid_size());
    event.set_ask_price(command.ask_price());
    event.set_ask_size(command.ask_size());
    event.set_exchange_time(command.exchange_time());

    return event;
  }

  std::vector<uint8_t> serialize(const toysequencer::TopOfBookEvent &event) const {
    std::string bytes;
    bytes.resize(event.ByteSizeLong());
    event.SerializeToArray(bytes.data(), static_cast<int>(bytes.size()));
    return std::vector<uint8_t>(bytes.begin(), bytes.end());
  }
};

} // namespace adapters
