#include "http_market_data_source.hpp"
#include "imarket_data_source.hpp"
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
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    auto log = [](const std::string &s) { std::cout << s << std::endl; };
    std::cout << "md: starting main" << std::endl;

    uint64_t md_instance_id = 3;

    std::unique_ptr<IMarketDataSource> src =
        std::make_unique<HttpSseMarketDataSource>(
            /*host=*/"127.0.0.1",
            /*port=*/"8000",
            /*path=*/"/stream/AAPL");

    MarketDataFeedApp md("239.255.0.2", 30002, 1, md_instance_id, log,
                         std::move(src));

    std::cout << "md: calling start()" << std::endl;
    md.start();
    std::cout << "md: start() returned" << std::endl;

    while (running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "md: calling stop()" << std::endl;
    md.stop();
    std::cout << "md: exited cleanly" << std::endl;
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "md error: " << e.what() << std::endl;
    return 1;
  }
}
