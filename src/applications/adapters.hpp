#pragma once

#include "generated/messages.pb.h"
#include <cstdint>
#include <string>
#include <vector>

namespace adapters {

struct TextCommandToTextEvent {
  toysequencer::TextEvent make_event(const toysequencer::TextCommand &command,
                                     uint64_t seq, uint64_t sender_id,
                                     uint64_t ts) const {
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
