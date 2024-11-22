#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include <iostream>

#include "bus.h"
#include "cache.h"

class Bus;

class Memory {
public:
    // load from address: returns {number of cycles, whether it's a cache hit, previous cache state, current cache state}
    std::tuple<int, bool, CacheState, CacheState> load(uint32_t address, Bus* bus);
    // store to address: returns {number of cycles, whether it's a cache hit, previous cache state, current cache state}
    std::tuple<int, bool, CacheState, CacheState> store(uint32_t address, Bus* bus);
    // compute the {tag, set index, offset}
    [[nodiscard]] std::tuple<uint32_t, uint32_t, uint32_t> compute_tag_idx_offset(uint32_t address) const;
    // process bus signal sent from another processor
    BusResponse process_signal_from_bus(BusMessage message, uint32_t address, Bus* bus);

    Memory(int _index, int cache_size, int associativity, int block_size, int address_bits, Protocol _protocol);
private:
    Protocol protocol;
    int core_index;

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
