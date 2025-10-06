#pragma once

#include "../application.hpp"
#include "abstract/imarket_data_source.hpp"
#include "abstract/md_notifier.hpp"
#include "core/command_sender.hpp"
#include "utils/instanceid_utils.hpp"
#include "utils/md_utils.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <generated/messages.pb.h>

class MarketDataFeedApp : public Application, public ICommandSender<MarketDataFeedApp>, public MdNotifier {
public:
  MarketDataFeedApp(const std::string &cmd_addr, uint16_t cmd_port, uint8_t ttl,
                    std::function<void(const std::string &)> log, std::unique_ptr<IMarketDataSource> src)
      : ICommandSender<MarketDataFeedApp>(cmd_addr, cmd_port, ttl), log_(std::move(log)), source_(std::move(src)) {}

  void notify(const std::string &data) override {
    toysequencer::TopOfBookCommand cmd =
        MDUtils::parse_json_tob(data, get_instance_id(), InstanceIdUtils::get_instance_id("SEQ"));
    this->send_command(cmd, get_instance_id());
  }

  void send_command(const toysequencer::TopOfBookCommand &command, const uint64_t sender_id) {
    std::string bytes = command.SerializeAsString();
    std::vector<uint8_t> data(bytes.begin(), bytes.end());
    this->send_m(data);
  }

  void start() override {
    if (!source_)
      return;
    source_->register_callback([this](const std::string &data) { this->notify(data); });
    source_->start();
  }

  void stop() override {
    if (source_)
      source_->stop();
  }

  uint64_t get_instance_id() const override { return InstanceIdUtils::get_instance_id("MD"); }

private:
  std::function<void(const std::string &)> log_;
  std::unique_ptr<IMarketDataSource> source_;
};
