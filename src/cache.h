#ifndef CACHE_H
#define CACHE_H
#include <list>

class LRUSet {
public:
    // returns true if tag is present
    bool write(uint32_t tag);
    // returns true if tag is present
    bool read(uint32_t tag);
    // returns true if the least recently used tag is evicted
    bool allocate(uint32_t tag, bool is_write);

    explicit LRUSet(int associativity);
private:
    int max_size;
    std::list<std::pair<uint32_t, bool>> tags;
    std::unordered_map<uint32_t, std::list<std::pair<uint32_t, bool>>::iterator> map;
};

#endif //CACHE_H