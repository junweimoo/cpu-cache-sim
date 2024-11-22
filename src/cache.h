#ifndef CACHE_H
#define CACHE_H

#include <list>
#include "enums.h"

class Bus;

class LRUSet {
public:
    // return state of block with tag
    CacheState get_state(uint32_t tag);
    // returns {previous state, whether another copy of this line is present, current state}
    std::tuple<CacheState, BusResponse, CacheState> write(uint32_t tag, Bus* bus, uint32_t address, int sender_idx);
    // returns {previous state, whether another copy of this line is present, current_state}
    std::tuple<CacheState, BusResponse, CacheState> read(uint32_t tag, Bus* bus, uint32_t address, int sender_idx);
    // returns true if the least recently used tag is evicted
    std::tuple<bool, BusResponse> allocate(uint32_t tag, bool is_write, Bus* bus, uint32_t address, int sender_idx);
    // process bus signal according to protocol
    BusResponse process_signal_from_bus(uint32_t tag, BusMessage message, Bus* bus, uint32_t address, int sender_idx);
    // get string name of cache state for debugging
    static std::string get_cache_state_str(CacheState state);

    explicit LRUSet(int associativity, Protocol _protocol);
private:
    Protocol protocol;
    int max_size;
    // list of pairs of {tag, state}
    std::list<std::pair<uint32_t, CacheState>> tags;
    // map from tag to iterator pointing to the pair of {tag, state} in the list of tags
    std::unordered_map<uint32_t, std::list<std::pair<uint32_t, CacheState>>::iterator> map;
};

#endif //CACHE_H