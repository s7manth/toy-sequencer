#include "imarket_data_source.hpp"
#include "market_data.hpp"
#include "market_data_feed.hpp"
#include <csignal>
#include <iostream>
#include <memory>

static std::atomic<bool> running{true};
static void handle_signal(int) { running.store(false); }

int main() {
  try {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    auto log = [](const std::string &s) { std::cout << s << std::endl; };

    // Instance ids can be parameterized later; use 3 for MD
    uint64_t md_instance_id = 3;

    // Choose a data source; default to simulated here
    std::unique_ptr<IMarketDataSource> src =
        std::make_unique<HttpSseMarketDataSource>(
            /*host=*/"127.0.0.1",
            /*port=*/"8000",
            /*path=*/"/stream/AAPL");

    MarketDataFeedApp md("239.255.0.1", 30001, 1, md_instance_id, log, std::move(src));

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
