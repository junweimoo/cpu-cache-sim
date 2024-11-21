#include "cache.h"
#include "bus.h"

LRUSet::LRUSet(int associativity) : max_size(associativity) {
}

bool LRUSet::allocate(uint32_t tag, bool is_write, Bus* bus) {
    auto it = map.find(tag);

    if (it != map.end()) {
        // tag is already in the set
        return false;
    }

    bool flushed = false;
    if (tags.size() == max_size) {
        // Evict the least recently used element from the back of the tags list
        std::pair<uint32_t, CacheState> p = tags.back();
        int lru_tag = p.first;
        CacheState state = p.second;
        tags.pop_back();
        map.erase(lru_tag);

        if (state == Modified) {
            flushed = true;
        }
    }

    // Insert the new element at the front
    tags.push_front({tag, is_write ? Modified : Exclusive});
    map[tag] = tags.begin();

    return flushed;
}

bool LRUSet::write(uint32_t tag, Bus* bus) {
    auto map_iter = map.find(tag);

    if (map_iter == map.end()) {
        // tag is not in the set
        return false;
    }

    // set dirty bit
    auto tags_iter = map_iter->second;
    tags_iter->second = Modified;

    // move the looked up tag to the front of the tags list
    tags.splice(tags.begin(), tags, tags_iter);
    return true;
}

bool LRUSet::read(uint32_t tag, Bus* bus) {
    auto it = map.find(tag);

    if (it == map.end()) {
        // tag is not in the set
        return false;
    }

    // move the looked up tag to the front of the tags list
    tags.splice(tags.begin(), tags, it->second);
    return true;
}

void LRUSet::process_signal_from_bus(uint32_t tag, BusMessage message) {
    auto map_iter = map.find(tag);
    auto tags_iter = map_iter->second;
    CacheState current_state = tags_iter->second;

    switch (message) {
        case BusMessage::Read:
            if (current_state == Modified) {
                tags_iter->second = Shared;
            } else if (current_state == Exclusive) {
                tags_iter->second = Shared;
            }
            break;
        case BusMessage::Invalidate:
            tags_iter->second = Invalid;
            break;
        case BusMessage::ReadExclusive:
            if (current_state == Modified) {
                tags_iter->second = Invalid;
            } else if (current_state == Exclusive) {
                tags_iter->second = Invalid;
            } else if (current_state == Shared) {
                tags_iter->second = Invalid;
            }
            break;
        case BusMessage::WriteBack:
            break;
    }
}