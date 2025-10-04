#pragma once

#include <functional>
#include <string>
#include <vector>

class IMarketDataSource {
public:
  virtual ~IMarketDataSource() = default;
  virtual void start() = 0;
  virtual void stop() = 0;

  using MdCallback = std::function<void(const std::string &)>;
  std::vector<MdCallback> callbacks{};

  void register_callback(MdCallback cb) {
    callbacks.emplace_back(std::move(cb));
  }

  void on_top_of_book(const std::string &data) {
    for (auto &cb : callbacks) {
      cb(data);
    }
  };
};
