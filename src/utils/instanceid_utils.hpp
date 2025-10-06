#pragma once

#include <string>
#include <unordered_map>

class InstanceIdUtils {
private:
  static const std::unordered_map<std::string, uint64_t> instance_id_map_;

public:
  static uint64_t get_instance_id(const std::string &instance_name) { return instance_id_map_.at(instance_name); }
};

inline const std::unordered_map<std::string, uint64_t> InstanceIdUtils::instance_id_map_{
    {"SEQ", 0}, {"SCRAPPY", 1}, {"PING", 2}, {"PONG", 3}, {"MD", 4}};
