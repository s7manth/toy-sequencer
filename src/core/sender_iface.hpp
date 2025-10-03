#pragma once

#include <cstdint>
#include <vector>

class ISender {
public:
  virtual ~ISender() = default;
  virtual bool send_m(const std::vector<uint8_t> &data) = 0;
  virtual bool send_m(const std::vector<uint8_t> &data, uint8_t ttl) = 0;
};
