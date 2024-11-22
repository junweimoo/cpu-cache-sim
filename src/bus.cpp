#include "bus.h"
#include "memory.h"

Bus::Bus(int _block_size) : total_invalidations(0), total_traffic(0), block_size(_block_size) {}

BusResponse Bus::broadcast(BusMessage message, uint32_t address, int sender_idx, CacheState sender_cache_state) {
    switch(message) {
    case ReadExclusive:
        // total_traffic += block_size;
    break;
    case Read:
        // total_traffic += block_size;
    break;
    case WriteBack:
        // traffic to write back to memory
        total_traffic += block_size;
    break;
    case Invalidate:
        total_invalidations++;
    break;
    default:
    }

    BusResponse response = NoResponse;
    for (int i = 0; i < memory_blocks.size(); i++) {
        // broadcast to other memory blocks apart from sender
        if (i == sender_idx) continue;

        if (memory_blocks[i]->process_signal_from_bus(message, address) == HasCopy) {
            // this cache address is also in another processor's cache
            response = HasCopy;
            if (message == ReadExclusive) {
                total_invalidations++;
            }
        }
    }

    if ((message == ReadExclusive || message == Read) &&
        response == HasCopy &&
        (sender_cache_state == Invalid || sender_cache_state == NotPresent)) {
        // traffic for cache-to-cache sharing
        total_traffic++;
    }

    return response;
}

long Bus::get_total_traffic() const {
    return total_traffic * block_size;
}

long Bus::get_total_invalidations() const {
    return total_invalidations;
}

void Bus::connect_memory(Memory* mem) {
    memory_blocks.push_back(mem);
}
