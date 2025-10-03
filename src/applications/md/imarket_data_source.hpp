#pragma once

#include "md_notifier.hpp"
#include <string>
#include <vector>
#include <functional>


class IMarketDataSource {
    public:
      virtual ~IMarketDataSource() = default;
      virtual void start() = 0;
      virtual void stop() = 0;
    
    std::vector<std::reference_wrapper<MdNotifier>> notifiers{};
    
    void register_notifier(MdNotifier &notifier) {
        notifiers.emplace_back(notifier);
    }
    
    void on_top_of_book(const std::string &data) {
        for (auto &notifier: notifiers) {
            notifier.get().notify(data);
        }
    };
};
