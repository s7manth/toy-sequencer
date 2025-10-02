#include "../support/test_harness.hpp"
#include "applications/sequencer.hpp"
#include <cassert>
#include <iostream>

int main() {
  std::cout << "=== Test: Instance ID Assignment and Uniqueness ==="
            << std::endl;

  TestHarness harness;
  auto &stubSender = harness.get_stub_sender();
  Sequencer seq(stubSender);

  uint64_t id1 = seq.assign_instance_id();
  uint64_t id2 = seq.assign_instance_id();
  uint64_t id3 = seq.assign_instance_id();

  assert(id1 == 1);
  assert(id2 == 2);
  assert(id3 == 3);

  std::cout << "✓ Instance IDs assigned correctly: " << id1 << ", " << id2
            << ", " << id3 << std::endl;
  std::cout << "✓ Test PASSED" << std::endl;
  return 0;
}
