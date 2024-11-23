#include "memory.h"

#include "bus.h"
#include "config.h"

Memory::Memory(int _index, int cache_size, int associativity, int block_size, int address_bits = 32, Protocol _protocol = MESI) :
        cache_size(cache_size), associativity(associativity), block_size(block_size) {
    core_index = _index;
    protocol = _protocol;

    int num_sets = cache_size / (block_size * associativity);
    cache.resize(num_sets, LRUSet(associativity, _protocol));

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

BusResponse Memory::process_signal_from_bus(BusMessage message, uint32_t address, Bus* bus) {
    uint32_t offset, set_index, tag;
    std::tie(offset, set_index, tag) = compute_tag_idx_offset(address);
    LRUSet& cache_set = cache[set_index];
    return cache_set.process_signal_from_bus(tag, message, bus, address, core_index);
}

std::tuple<int, bool, CacheState, CacheState> Memory::load(uint32_t address, Bus* bus) {
    uint32_t offset, set_index, tag;
    std::tie(offset, set_index, tag) = compute_tag_idx_offset(address);

    LRUSet& cache_set = cache[set_index];
    CacheState prev_state, curr_state;
    BusResponse response;
    std::tie(prev_state, response, curr_state) = cache_set.read(tag, bus, address, core_index);

    // MESI
    if (prev_state == Modified || prev_state == Exclusive || prev_state == Shared) {
        // cache hit -> load from cache
        return {Config::CACHE_HIT_TIME, true, prev_state, curr_state};
    } else if (prev_state == Invalid) {
        // cache has been invalidated -> load from memory
        if (response == BusResponseShared) {
            return {Config::SEND_WORD_TIME + Config::CACHE_HIT_TIME, false, prev_state, curr_state};
        } else if (response == BusResponseDirty) {
            return {Config::SEND_WORD_TIME + Config::MEM_FLUSH_TIME + Config::CACHE_HIT_TIME, false, prev_state, curr_state};
        } else {
            return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME, false, prev_state, curr_state};
        }
    }

    // Dragon
    if (prev_state == ExclusiveDragon || prev_state == SharedClean || prev_state == SharedModified || prev_state == Dirty) {
        // cache hit -> load from cache
        return {Config::CACHE_HIT_TIME, true, prev_state, curr_state};
    }

    // Not present in cache -> allocate
    bool is_evicted;
    std::tie(is_evicted, response) = cache_set.allocate(tag, false, bus, address, core_index);
    curr_state = cache_set.get_state(tag);
    int cycles = 0;

    if (protocol == MESI) {
        if (response == BusResponseShared) {
            cycles = Config::SEND_WORD_TIME + Config::CACHE_HIT_TIME;
        } else if (response == BusResponseDirty) {
            cycles = Config::SEND_WORD_TIME + Config::MEM_FLUSH_TIME + Config::CACHE_HIT_TIME;
        } else {
            cycles = Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME;
        }
    }

    if (protocol == Dragon) {
        if (response == BusResponseShared) {
            cycles = Config::SEND_WORD_TIME + Config::CACHE_HIT_TIME;
        } else if (response == BusResponseDirty) {
            cycles = Config::SEND_WORD_TIME + Config::MEM_FLUSH_TIME + Config::CACHE_HIT_TIME;
        } else {
            cycles = Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME;
        }
    }

    if (is_evicted) cycles += Config::MEM_FLUSH_TIME;
    return {cycles, false, prev_state, curr_state};
}

std::tuple<int, bool, CacheState, CacheState> Memory::store(uint32_t address, Bus* bus) {
    uint32_t offset, set_index, tag;
    std::tie(offset, set_index, tag) = compute_tag_idx_offset(address);

    LRUSet& cache_set = cache[set_index];
    CacheState prev_state, curr_state;
    BusResponse response;
    std::tie(prev_state, response, curr_state) = cache_set.write(tag, bus, address, core_index);

    // MESI
    if (prev_state == Modified || prev_state == Exclusive || prev_state == Shared) {
        // cache hit -> write to cache
        return {Config::CACHE_HIT_TIME, true, prev_state, curr_state};
    } else if (prev_state == Invalid) {
        // if (response == HasCopy) {
        //     return {Config::SEND_WORD_TIME + Config::CACHE_HIT_TIME, false, prev_state, curr_state};
        // }
        if (response == BusResponseShared) {
            return {Config::SEND_WORD_TIME + Config::CACHE_HIT_TIME, false, prev_state, curr_state};
        } else if (response == BusResponseDirty) {
            return {Config::SEND_WORD_TIME + Config::MEM_FLUSH_TIME + Config::CACHE_HIT_TIME, false, prev_state, curr_state};
        } else {
            return {Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME, false, prev_state, curr_state};
        }
    }

    // Dragon
    if (prev_state == ExclusiveDragon || prev_state == Dirty) {
        // cache hit -> load from cache
        return {Config::CACHE_HIT_TIME, true, prev_state, curr_state};
    } else if (prev_state == SharedClean || prev_state == SharedModified) {
        // cache hit -> send update to other caches + load from cache
        return {Config::SEND_WORD_TIME + Config::CACHE_HIT_TIME, true, prev_state, curr_state};
    }

    // cache miss -> allocate
    bool is_evicted;
    std::tie(is_evicted, response) = cache_set.allocate(tag, false, bus, address, core_index);
    curr_state = cache_set.get_state(tag);
    int cycles = 0;

    if (protocol == MESI) {
        if (response == BusResponseShared) {
            cycles = Config::SEND_WORD_TIME + Config::CACHE_HIT_TIME;
        } else if (response == BusResponseDirty) {
            cycles = Config::SEND_WORD_TIME + Config::MEM_FLUSH_TIME + Config::CACHE_HIT_TIME;
        } else {
            cycles = Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME;
        }
    }

    if (protocol == Dragon) {
        if (response == BusResponseShared) {
            cycles = 2 * Config::SEND_WORD_TIME + Config::CACHE_HIT_TIME;
        } else if (response == BusResponseDirty) {
            cycles = 2 * Config::SEND_WORD_TIME + Config::MEM_FLUSH_TIME + Config::CACHE_HIT_TIME;
        } else {
            cycles = Config::MEM_FETCH_TIME + Config::CACHE_HIT_TIME;
        }
    }

    if (is_evicted) cycles += Config::MEM_FLUSH_TIME;
    return {cycles, false, prev_state, curr_state};
}

std::tuple<uint32_t, uint32_t, uint32_t> Memory::compute_tag_idx_offset(uint32_t address) const {
    uint32_t offset = address & offset_mask;
    uint32_t set_index = address & set_index_mask >> offset_bits;
    uint32_t tag = address & tag_mask >> (offset_bits + set_index_bits);
    return std::make_tuple(offset, set_index, tag);
}