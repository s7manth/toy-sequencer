#include "../../utils/env_utils.hpp"
#include "messages.pb.h"
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
    EnvUtils::load_env();

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    std::string output = std::getenv("SCRAPPY_FILE");
    std::string mcast_addr = std::getenv("EVENTS_ADDR");
    uint16_t port = std::stoi(std::getenv("EVENTS_PORT"));

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

    scrappy.subscribe<toysequencer::TopOfBookEvent>(toysequencer::TOB_EVENT);
    scrappy.subscribe<toysequencer::TextEvent>(toysequencer::TEXT_EVENT);

    std::cout << "scrappy listening on " << mcast_addr << ":" << port << ", writing to " << output << std::endl;

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
