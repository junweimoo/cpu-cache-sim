#ifndef CACHE_H
#define CACHE_H
#include <list>

class LRUSet {
public:
    // returns true if tag is present
    bool lookup(uint32_t tag);
    // returns true if the least recently used tag is evicted
    bool insert(uint32_t tag);

    explicit LRUSet(int associativity);
private:
    int max_size;
    std::list<uint32_t> tags;
    std::unordered_map<uint32_t, std::list<uint32_t>::iterator> map;
};

#endif //CACHE_H