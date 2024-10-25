#include "bus.h"
#include "memory.h"

Bus::Bus(int _block_size) : total_invalidations(0), total_traffic(0), block_size(_block_size) {}

void Bus::broadcast(BusMessage message_type, uint32_t address) {
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
}

long Bus::get_total_traffic() const {
    return total_traffic;
}

long Bus::get_total_invalidations() const {
    return total_invalidations;
}

void Bus::connect_memory(Memory* mem) {
    memory = mem;
}
