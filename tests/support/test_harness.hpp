#pragma once

#include "../stubs/stub_multicast_sender.hpp"

class TestHarness {
public:
  TestHarness() = default;

  StubMulticastSender &get_stub_sender() { return stub_sender_; }

private:
  StubMulticastSender stub_sender_{};
};


