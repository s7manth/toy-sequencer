#include "receiver.hpp"
#include "generated/messages.pb.h"
#include <iostream>

Receiver::Receiver(
    std::function<void(const toysequencer::TextEvent &)> on_event,
    uint64_t instance_id)
    : on_event_(std::move(on_event)), instance_id_(instance_id) {}

void Receiver::on_datagram(const uint8_t *data, size_t len) {
  try {
    toysequencer::TextEvent event;
    if (!event.ParseFromArray(data, static_cast<int>(len))) {
      std::cerr << "Failed to parse TextEvent from datagram" << std::endl;
      return;
    }

    // Filter events based on TIN (Target Instance ID)
    if (event.tin() != instance_id_) {
      // This event is not for this instance; drop silently
      return;
    }

    if (event.seq() < expected_seq_) {
      // Duplicate/old for this receiver; drop quietly.
      return;
    }

    if (event.seq() == expected_seq_) {
      on_event_(event);
      expected_seq_++;
      return;
    }

    // event.seq() > expected_seq_
    std::cout << "Out of order event received. Expected: " << expected_seq_
              << ", Got: " << event.seq() << std::endl;
    on_event_(event);

  } catch (const std::exception &e) {
    std::cerr << "Receiver error: " << e.what() << std::endl;
  }
}
