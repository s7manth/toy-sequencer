#include "scrappy.hpp"
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

static std::atomic<bool> running{true};
static void handle_signal(int) { running.store(false); }

int main(int argc, char **argv) {
  try {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

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

    scrappy.subscribe<toysequencer::TopOfBookEvent>();
    scrappy.subscribe<toysequencer::TextEvent>();

    std::cout << "scrappy listening on " << mcast_addr << ":" << port
              << ", writing to " << output << std::endl;

    while (running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    scrappy.stop();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "scrappy error: " << e.what() << std::endl;
    return 1;
  }
}
