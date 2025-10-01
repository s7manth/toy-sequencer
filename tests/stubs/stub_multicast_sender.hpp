#pragma once

#include "core/sender_iface.hpp"
#include <vector>

class StubMulticastSender : public ISender {
public:
  bool send(const std::vector<uint8_t> &data) { return record(data); }
  bool send(const std::vector<uint8_t> &data, uint8_t) { return record(data); }

  const std::vector<std::vector<uint8_t>> &get_sent_datagrams() const {
    return sent_;
  }

private:
  bool record(const std::vector<uint8_t> &data) {
    sent_.push_back(data);
    return true;
  }

  std::vector<std::vector<uint8_t>> sent_;
};


