#pragma once

#include "core/command_sender.hpp"
#include "imarket_data_source.hpp"
#include "md_notifier.hpp"
#include "md_utils.hpp"
#include <functional>
#include <memory>
#include <string>
#include <cstdint>

#include <generated/messages.pb.h>

class MarketDataFeedApp : public ICommandSender<MarketDataFeedApp>, public MdNotifier {
public:
  MarketDataFeedApp(const std::string &multicast_address, uint16_t port, uint8_t ttl,
                    uint64_t instance_id,
                    std::function<void(const std::string &)> log,
                    std::unique_ptr<IMarketDataSource> src)
      : ICommandSender<MarketDataFeedApp>(multicast_address, port, ttl),
        log_(std::move(log)), source_(std::move(src)),
        instance_id_(instance_id) {
            source_->register_notifier(*this);
        }

  void start() {
    if (!source_)
      return;
    source_->start();
  }

  void stop() {
    if (source_)
      source_->stop();
  }

  void notify(const std::string &data) override {
    toysequencer::TopOfBookCommand cmd = MDUtils::parse_json_tob(data, 0);
    this->send_command(cmd, instance_id_);
  }

  void send_command(const toysequencer::TopOfBookCommand &command,
                    const uint64_t sender_id) {
    this->send(command, sender_id);
  }

private:
  std::function<void(const std::string &)> log_;
  std::unique_ptr<IMarketDataSource> source_;
  uint64_t instance_id_;
};
