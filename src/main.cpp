#include "applications/receiver.hpp"
#include "applications/sequencer.hpp"
#include "core/multicast_sender.hpp"
#include <chrono>
#include <iostream>

int main(int, char **) {
  try {
    MulticastSender sender("239.255.0.1", 30001);
    Sequencer sequencer(sender);

    auto message_handler = [](const Message &msg) {
      std::cout << "Received message - Seq: " << msg.header.seq
                << ", Type: " << msg.header.type
                << ", Size: " << msg.header.payload_size << std::endl;
    };

    Receiver receiver(message_handler);

    std::string payload = "Hello, world!";
    std::vector<uint8_t> payload_bytes(payload.begin(), payload.end());
    uint64_t seq = sequencer.publish(1, payload_bytes);

    std::cout << "Published message with sequence: " << seq << std::endl;

    Message msg;
    msg.header.seq = seq;
    msg.header.timestamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    msg.header.type = 1;
    msg.header.payload_size = payload_bytes.size();
    msg.payload = payload_bytes;

    auto serialized = msg.serialize();
    receiver.on_datagram(serialized.data(), serialized.size());

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
