#include "receiver.hpp"
#include <iostream>
#include <vector>

Receiver::Receiver(std::function<void(const Message &)> on_message)
    : on_message_(std::move(on_message)) {}

void Receiver::on_datagram(const uint8_t *data, size_t len) {
  try {
    std::vector<uint8_t> buffer(data, data + len);

    Message msg = Message::deserialize(buffer);
    if (msg.header.seq == expected_seq_) {
      // in order message
      on_message_(msg);
      expected_seq_++;
    } else if (msg.header.seq > expected_seq_) {
      // out of order message - buffer it for now
      // TODO: Implement proper buffering and gap detection
      std::cout << "Out of order message received. Expected: " << expected_seq_
                << ", Got: " << msg.header.seq << std::endl;
      on_message_(msg);
    } else {
      // duplicate or old message
      std::cout << "Duplicate/old message received. Expected: " << expected_seq_
                << ", Got: " << msg.header.seq << std::endl;
    }

  } catch (const std::exception &e) {
    std::cerr << "Failed to deserialize message: " << e.what() << std::endl;
  }
}
