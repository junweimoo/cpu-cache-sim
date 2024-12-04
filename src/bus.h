#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <mutex>
#include <vector>
#include "enums.h"

class Memory;

class Bus {
public:
    BusResponse broadcast(BusMessage message, uint32_t address, int sender_idx, CacheState sender_cache_state);
    long get_total_traffic() const;
    long get_total_invalidations() const;
    void connect_memory(Memory* mem);

    Bus(int _block_size);
private:
    // total traffic from read, read exclusive, write back in bytes
    long long total_traffic;
    long total_invalidations_updates;
    int block_size;
    std::mutex mtx;

    std::vector<Memory*> memory_blocks;
};

#endif //BUS_H
