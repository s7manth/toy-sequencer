#include "sequencer.hpp"
#include "message.hpp"
#include <iostream>

Sequencer::Sequencer(MulticastSender& sender) : sender_(sender) {
}

uint64_t Sequencer::publish(uint32_t type, const std::vector<uint8_t>& payload) {
    uint64_t seq = next_seq_.fetch_add(1);
    
    // construct a message
    Message msg;
    msg.header.seq = seq;
    msg.header.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    msg.header.type = type;
    msg.header.payload_size = payload.size();
    msg.payload = payload;
    
    auto serialized = msg.serialize();
    bool success = sender_.send(serialized);
    
    if (!success) {
        std::cerr << "Failed to send message with sequence " << seq << std::endl;
    }
    
    return seq;
}

void Sequencer::retransmit(uint64_t from_seq, uint64_t to_seq) {
    // TODO: implement retransmission logic
    // this would typically involve:
    // 1. looking up messages in the persistent log (WAL)
    // 2. resending them via multicast
    std::cout << "Retransmit requested for sequences " << from_seq << " to " << to_seq << std::endl;
}
