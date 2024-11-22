#include "memory.h"

#include "bus.h"
#include "config.h"

Memory::Memory(int _index, int cache_size, int associativity, int block_size, int address_bits = 32) :
        cache_size(cache_size), associativity(associativity), block_size(block_size) {
    index = _index;

    int num_sets = cache_size / (block_size * associativity);
    cache.resize(num_sets, LRUSet(associativity));

    offset_bits = std::log2(block_size);
    set_index_bits = std::log2(num_sets);
    tag_bits = address_bits - offset_bits - set_index_bits;

    offset_mask = (1 << offset_bits) - 1;                                 // e.g. 00000000000000000000000000001111
    set_index_mask = ((1 << set_index_bits) - 1) << offset_bits;          // e.g. 00000000000000000000001111110000
    tag_mask = ((1 << tag_bits) - 1) << (offset_bits + set_index_bits);   // e.g. 11111111111111111111110000000000

    std::cout << "Memory initialized. Offset: " << offset_bits << " bits. Set Index: "
              << set_index_bits << " bits. Tag: " << tag_bits << " bits. "
              << num_sets << " sets, " << associativity << "-way associative." << std::endl;
}

BusResponse Memory::process_signal_from_bus(BusMessage message, uint32_t address) {
    uint32_t offset, set_index, tag;
    std::tie(offset, set_index, tag) = compute_tag_idx_offset(address);
    LRUSet& cache_set = cache[set_index];
    return cache_set.process_signal_from_bus(tag, message);
}

std::pair<int, bool> Memory::load(uint32_t address, Bus* bus) {
    uint32_t offset, set_index, tag;
    std::tie(offset, set_index, tag) = compute_tag_idx_offset(address);

    LRUSet& cache_set = cache[set_index];
    const CacheState prev_state = cache_set.read(tag, bus, address, index);

    if (cache_set.get_state(tag) != NotPresent)
        std::cout << index << " [load] prev_state:" << LRUSet::get_cache_state_str(prev_state)
                << " curr_state:" << LRUSet::get_cache_state_str(cache_set.get_state(tag)) << std::endl;

    if (prev_state == Modified || prev_state == Exclusive || prev_state == Shared) {
        // cache hit -> load from cache
        return {Config::CACHE_HIT_TIME, true};
    } else if (prev_state == Invalid) {
        // cache has been invalidated -> load from memory
        return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME, false};
    }

    // cache miss -> allocate
    if (cache_set.allocate(tag, false, bus, address, index)) {
        std::cout << index << " [load] prev_state:" << LRUSet::get_cache_state_str(prev_state)
                << " curr_state:" << LRUSet::get_cache_state_str(cache_set.get_state(tag)) << std::endl;

        // least recently used tag flushed
        // cycles = fetch from memory + from cache + flush dirty block to memory
        return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME + Config::MEM_FLUSH_TIME, false};
    } else {
        std::cout << index << " [load] prev_state:" << LRUSet::get_cache_state_str(prev_state)
                << " curr_state:" << LRUSet::get_cache_state_str(cache_set.get_state(tag)) << std::endl;

        // no flushes (either LRU has no dirty bit, or empty space remaining in the set)
        // cycles = fetch from memory + from cache
        return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME, false};
    }
}

std::pair<int, bool> Memory::store(uint32_t address, Bus* bus) {
    uint32_t offset, set_index, tag;
    std::tie(offset, set_index, tag) = compute_tag_idx_offset(address);

    LRUSet& cache_set = cache[set_index];
    const CacheState prev_state = cache_set.write(tag, bus, address, index);

    if (cache_set.get_state(tag) != NotPresent)
        std::cout << index << " [store] prev_state:" << LRUSet::get_cache_state_str(prev_state)
                << " curr_state:" << LRUSet::get_cache_state_str(cache_set.get_state(tag)) << std::endl;

    if (prev_state == Modified || prev_state == Exclusive || prev_state == Shared) {
        // cache hit -> write to cache
        return {Config::CACHE_HIT_TIME, true};
    } else if (prev_state == Invalid) {
        return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME, false};
    }

    // cache miss -> allocate
    if (cache_set.allocate(tag, true, bus, address, index)) {
        std::cout << index << " [load] prev_state:" << LRUSet::get_cache_state_str(prev_state)
                << " curr_state:" << LRUSet::get_cache_state_str(cache_set.get_state(tag)) << std::endl;

        // least recently used tag flushed
        // cycles = fetch from memory + from cache + flush dirty block to memory
        return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME + Config::MEM_FLUSH_TIME, false};
    } else {
        std::cout << index << " [load] prev_state:" << LRUSet::get_cache_state_str(prev_state)
                << " curr_state:" << LRUSet::get_cache_state_str(cache_set.get_state(tag)) << std::endl;

        // no flushes (either LRU has no dirty bit, or empty space remaining in the set)
        // cycles = fetch from memory + from cache
        return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME, false};
    }
}

std::tuple<uint32_t, uint32_t, uint32_t> Memory::compute_tag_idx_offset(uint32_t address) const {
    uint32_t offset = address & offset_mask;
    uint32_t set_index = address & set_index_mask >> offset_bits;
    uint32_t tag = address & tag_mask >> (offset_bits + set_index_bits);
    return std::make_tuple(offset, set_index, tag);
}