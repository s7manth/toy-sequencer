#include "../../utils/env_utils.hpp"
#include "abstract/imarket_data_source.hpp"
#include "impl/http_market_data_source.hpp"
#include "market_data_feed.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

static std::atomic<bool> running{true};
static void handle_signal(int) { running.store(false); }

int main() {
  try {
    EnvUtils::load_env();

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    auto log = [](const std::string &s) { std::cout << s << std::endl; };
    std::cout << "md: starting main" << std::endl;

    uint64_t md_instance_id = 3;

    std::unique_ptr<IMarketDataSource> src = std::make_unique<HttpSseMarketDataSource>(
        std::getenv("MD_SOURCE_HOST"), std::getenv("MD_SOURCE_PORT"), std::getenv("MD_SOURCE_PATH"));

    const std::string cmd_addr = std::getenv("CMD_ADDR");
    const uint16_t cmd_port = std::stoi(std::getenv("CMD_PORT"));
    const uint8_t mcast_ttl = 1;

    MarketDataFeedApp md(cmd_addr, cmd_port, mcast_ttl, log, std::move(src));

    md.start();

    while (running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    md.stop();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "md error: " << e.what() << std::endl;
    return 1;
  }
}
