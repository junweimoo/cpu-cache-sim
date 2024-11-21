#include "bus.h"
#include "memory.h"

Bus::Bus(int _block_size) : total_invalidations(0), total_traffic(0), block_size(_block_size) {}

void Bus::broadcast(BusMessage message_type, uint32_t address, int sender_idx) {
    switch(message_type) {
        case ReadExclusive:
            total_traffic += block_size;
        break;
        case Read:
            total_traffic += block_size;
        break;
        case WriteBack:
            total_traffic += block_size;
        break;
        case Invalidate:
            total_invalidations++;
        break;
        default:
    }

    for (int i = 0; i < memory_blocks.size(); i++) {
        // broadcast to other memory blocks apart from sender
        if (i == sender_idx) continue;
        memory_blocks[i]->process_signal_from_bus(message_type, address);
    }
}

long Bus::get_total_traffic() const {
    return total_traffic;
}

long Bus::get_total_invalidations() const {
    return total_invalidations;
}

void Bus::connect_memory(Memory* mem) {
    memory_blocks.push_back(mem);
}
