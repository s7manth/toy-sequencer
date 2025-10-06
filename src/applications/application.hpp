#pragma once

#include <cstdint>

class Application {
public:
  virtual ~Application() = default;
  virtual void start() = 0;
  virtual void stop() = 0;

  virtual uint64_t get_instance_id() const = 0;
};
