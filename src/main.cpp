#include "applications/receiver.hpp"
#include "applications/sequencer.hpp"
#include "core/multicast_sender.hpp"
#include "generated/messages.pb.h"
#include <chrono>
#include <iostream>

int main(int, char **) {
  try {
    MulticastSender sender("239.255.0.1", 30001);
    Sequencer sequencer(sender);

    auto message_handler = [](const toysequencer::TextEvent &msg) {
      std::cout << "Received message - Seq: " << msg.seq()
                << ", Size: " << msg.text().size() << std::endl;
    };

    Receiver receiver(message_handler);

    std::string payload = "Hello, world!";
    std::vector<uint8_t> payload_bytes(payload.begin(), payload.end());
    toysequencer::TextCommand cmd;
    cmd.set_text(payload);
    uint64_t seq = sequencer.publish(cmd);

    std::cout << "Published message with sequence: " << seq << std::endl;

    toysequencer::TextEvent msg;
    msg.set_seq(seq);
    msg.set_timestamp(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count());
    msg.set_text(payload);

    std::string bytes;
    bytes.resize(msg.ByteSizeLong());
    msg.SerializeToArray(bytes.data(), static_cast<int>(bytes.size()));

    receiver.on_datagram(reinterpret_cast<const uint8_t *>(bytes.data()),
                         bytes.size());
    std::cout << "Received message - Seq: " << msg.seq()
              << ", Size: " << msg.text().size() << std::endl;

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
