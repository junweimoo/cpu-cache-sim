#include "cache.h"

LRUSet::LRUSet(int associativity) : max_size(associativity) {
}

bool LRUSet::insert(uint32_t tag) {
    auto it = map.find(tag);

    if (it != map.end()) {
        // tag is already in the set
        return false;
    }

    bool evicted = false;
    if (tags.size() == max_size) {
        // Evict the least recently used element from the back of the tags list
        int lru = tags.back();
        tags.pop_back();
        map.erase(lru);
        evicted = true;
    }

    // Insert the new element at the front
    tags.push_front(tag);
    map[tag] = tags.begin();

    return evicted;
}

bool LRUSet::lookup(uint32_t tag) {
    auto it = map.find(tag);

    if (it == map.end()) {
        // tag is not in the set
        return false;
    }

    // move the looked up tag to the front of the tags list
    tags.splice(tags.begin(), tags, it->second);
    return true;
}
