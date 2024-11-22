#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include <iostream>

#include "bus.h"
#include "cache.h"

class Bus;

class Memory {
public:
    std::pair<int, bool> load(uint32_t address, Bus* bus);
    std::pair<int, bool> store(uint32_t address, Bus* bus);
    std::tuple<uint32_t, uint32_t, uint32_t> computeTagIdxOffset(uint32_t address) const;
    BusResponse process_signal_from_bus(BusMessage message, uint32_t address);

    Memory(int _index, int cache_size, int associativity, int block_size, int address_bits);
private:
    int index;

    int cache_size;
    int associativity;
    int block_size;

    int offset_bits;
    int set_index_bits;
    int tag_bits;
    uint32_t offset_mask;
    uint32_t set_index_mask;
    uint32_t tag_mask;

    // LRU sets indexed by set index
    std::vector<LRUSet> cache;
};

#endif
