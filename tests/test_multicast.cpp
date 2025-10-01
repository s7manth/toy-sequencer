#include "applications/receiver.hpp"
#include "core/multicast_sender.hpp"
#include "msg/message.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  try {
    // Create sender
    MulticastSender sender("239.255.0.1", 30001);

    // Create receiver with message handler
    auto message_handler = [](const Message &msg) {
      std::cout << "Received multicast message:" << std::endl;
      std::cout << "  Sequence: " << msg.header.seq << std::endl;
      std::cout << "  Timestamp: " << msg.header.timestamp << std::endl;
      std::cout << "  Type: " << msg.header.type << std::endl;
      std::cout << "  Payload size: " << msg.header.payload_size << std::endl;
      std::cout << "  Payload: ";
      for (uint8_t byte : msg.payload) {
        std::cout << static_cast<char>(byte);
      }
      std::cout << std::endl << std::endl;
    };

    Receiver receiver(message_handler);

    // Send some test messages
    std::vector<std::string> test_messages = {
        "Hello, multicast world!", "This is message #2",
        "Testing UDP multicast", "Final test message"};

    std::cout << "Sending " << test_messages.size() << " multicast messages..."
              << std::endl;

    for (size_t i = 0; i < test_messages.size(); ++i) {
      // Create message
      Message msg;
      msg.header.seq = i + 1;
      msg.header.timestamp =
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch())
              .count();
      msg.header.type = 1;
      msg.header.payload_size = test_messages[i].size();
      msg.payload.assign(test_messages[i].begin(), test_messages[i].end());

      // Serialize and send
      auto serialized = msg.serialize();
      bool success = sender.send(serialized);

      if (success) {
        std::cout << "Sent message " << (i + 1) << ": " << test_messages[i]
                  << std::endl;

        // Simulate receiving the message
        receiver.on_datagram(serialized.data(), serialized.size());
      } else {
        std::cerr << "Failed to send message " << (i + 1) << std::endl;
      }

      // Small delay between messages
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Multicast test completed successfully!" << std::endl;
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
