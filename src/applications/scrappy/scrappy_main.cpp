#include "scrappy.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  try {
    std::string output = "sequenced_events.txt";
    std::string mcast_addr = "239.255.0.1";
    uint16_t port = 30001;

    if (argc > 1) {
      output = argv[1];
    }
    if (argc > 2) {
      mcast_addr = argv[2];
    }
    if (argc > 3) {
      port = static_cast<uint16_t>(std::stoi(argv[3]));
    }

    ScrappyApp scrappy(output, mcast_addr, port);
    scrappy.start();

    std::cout << "scrappy listening on " << mcast_addr << ":" << port
              << ", writing to " << output << std::endl;

    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    scrappy.stop();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "scrappy error: " << e.what() << std::endl;
    return 1;
  }
}
