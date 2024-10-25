#include "memory.h"
#include <iostream>

#include "config.h"

Memory::Memory(int cache_size, int associativity, int block_size, int address_bits = 32) :
        cache_size(cache_size), associativity(associativity), block_size(block_size) {
    int num_sets = cache_size / (block_size * associativity);
    cache.resize(num_sets, LRUSet(associativity));

    offset_bits = std::log2(block_size);
    set_index_bits = std::log2(num_sets);
    tag_bits = address_bits - offset_bits - set_index_bits;

    offset_mask = (1 << offset_bits) - 1;                                 // e.g. 00000000000000000000000000001111
    set_index_mask = ((1 << set_index_bits) - 1) << offset_bits;          // e.g. 00000000000000000000001111110000
    tag_mask = ((1 << tag_bits) - 1) << (offset_bits + set_index_bits);   // e.g. 11111111111111111111110000000000

    std::cout << "Memory initialized. Offset: " << offset_bits << " bits. Set Index: "
              << set_index_bits << " bits. Tag: " << tag_bits << " bits." << std::endl;
}

std::pair<int, bool> Memory::load(uint32_t address) {
    uint32_t offset, set_index, tag;
    std::tie(offset, set_index, tag) = computeTagIdxOffset(address);

    LRUSet& cache_set = cache[set_index];
    if (cache_set.read(tag)) {
        // cache hit -> load from cache
        return {Config::CACHE_HIT_TIME, true};
    }

    // cache miss -> allocate
    if (cache_set.allocate(tag, false)) {
        // least recently used tag evicted
        // cycles = fetch from memory + from cache + flush dirty block to memory
        return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME + Config::MEM_FLUSH_TIME, false};
    } else {
        // no evictions
        // cycles = fetch from memory + from cache
        return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME, false};
    }
}

std::pair<int, bool> Memory::store(uint32_t address) {
    uint32_t offset, set_index, tag;
    std::tie(offset, set_index, tag) = computeTagIdxOffset(address);

    LRUSet& cache_set = cache[set_index];
    if (cache_set.write(tag)) {
        // cache hit -> write to cache
        return {Config::CACHE_HIT_TIME, true};
    }

    // cache miss -> allocate
    if (cache_set.allocate(tag, true)) {

        // least recently used tag flushed
        // cycles = fetch from memory + from cache + flush dirty block to memory
        return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME + Config::MEM_FLUSH_TIME, false};
    } else {

        // no flushes (either LRU has no dirty bit, or empty space remaining in the set)
        // cycles = fetch from memory + from cache
        return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME, false};
    }
}

std::tuple<uint32_t, uint32_t, uint32_t> Memory::computeTagIdxOffset(uint32_t address) const {
    uint32_t offset = address & offset_mask;
    uint32_t set_index = address & set_index_mask >> offset_bits;
    uint32_t tag = address & tag_mask >> (offset_bits + set_index_bits);
    return std::make_tuple(offset, set_index, tag);
}