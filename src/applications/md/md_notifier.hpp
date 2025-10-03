#pragma once

#include <string>

class MdNotifier {
    public:
      virtual ~MdNotifier() = default;
      virtual void notify(const std::string &data);
};
