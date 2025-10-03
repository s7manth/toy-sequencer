#pragma once

#include "generated/messages.pb.h"
#include <simdjson.h>
#include <string>
#include <cstdint>

class MDUtils {
public:
    static toysequencer::TopOfBookCommand parse_json_tob(const std::string &json, uint64_t target_instance) {
        simdjson::ondemand::parser parser;
        simdjson::padded_string padded(json);
        simdjson::ondemand::document obj = parser.iterate(padded);

        std::string symbol;
        double bid_p = 0.0, ask_p = 0.0;
        uint64_t bid_s = 0, ask_s = 0;
        double ts_sec = 0.0;

        toysequencer::TopOfBookCommand out = toysequencer::TopOfBookCommand();

        {
            std::string_view sv;
            if (obj["symbol"].get(sv)) return out;
            symbol.assign(sv.data(), sv.size());
        }

        if (obj["bid_price"].get(bid_p)) return out;
        if (obj["ask_price"].get(ask_p)) return out;

        {
            double tmp = 0.0;
            if (obj["bid_size"].get(tmp)) return out;
            if (tmp < 0) return out;
            bid_s = static_cast<uint64_t>(tmp);
        }

        {
            double tmp = 0.0;
            if (obj["ask_size"].get(tmp)) return out;
            if (tmp < 0) return out;
            ask_s = static_cast<uint64_t>(tmp);
        }

        if (obj["timestamp"].get(ts_sec)) return out;

        uint64_t exch_ts = static_cast<uint64_t>(ts_sec * 1'000'000.0);

        out.Clear();
        out.set_tin(target_instance);
        out.set_symbol(symbol);
        out.set_bid_price(bid_p);
        out.set_bid_size(bid_s);
        out.set_ask_price(ask_p);
        out.set_ask_size(ask_s);
        out.set_exchange_time(exch_ts);

        return out;
    }
};
