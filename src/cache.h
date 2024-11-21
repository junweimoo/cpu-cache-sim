#ifndef CACHE_H
#define CACHE_H

#include <list>

#include "bus.h"

enum CacheState {
    Modified,
    Exclusive,
    Shared,
    Invalid
};

class Bus;

class LRUSet {
public:
    // returns true if tag is present
    bool write(uint32_t tag, Bus* bus);
    // returns true if tag is present
    bool read(uint32_t tag, Bus* bus);
    // returns true if the least recently used tag is evicted
    bool allocate(uint32_t tag, bool is_write, Bus* bus);
    // process bus signal according to protocol
    void process_signal_from_bus(uint32_t tag, BusMessage message);

    explicit LRUSet(int associativity);
private:
    int max_size;
    std::list<std::pair<uint32_t, CacheState>> tags;
    std::unordered_map<uint32_t, std::list<std::pair<uint32_t, CacheState>>::iterator> map;
};

#endif //CACHE_H