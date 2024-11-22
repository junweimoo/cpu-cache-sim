#include <string>

#include "cache.h"
#include "bus.h"

std::string LRUSet::get_cache_state_str(CacheState state) {
    switch (state) {
    case Modified:
        return "Modified";
    case Exclusive:
        return "Exclusive";
    case Shared:
        return "Shared";
    case Invalid:
        return "Invalid";
    case NotPresent:
        return "NotPresent";
    default:
        return "";
    }
}

LRUSet::LRUSet(int associativity) : max_size(associativity) {
}

CacheState LRUSet::get_state(uint32_t tag) {
    auto map_iter = map.find(tag);
    if (map_iter == map.end()) return NotPresent;
    return map_iter->second->second;
}


bool LRUSet::allocate(uint32_t tag, bool is_write, Bus* bus, uint32_t address, int sender_idx) {
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
            // Write back flushed element to memory via the bus
            bus->broadcast(WriteBack, address, sender_idx, state);
        }
    }

    // send bus signal
    CacheState stateOfNewLine;
    if (is_write) {
        bus->broadcast(ReadExclusive, address, sender_idx, NotPresent);
        stateOfNewLine = Modified;
    } else {
        BusResponse response = bus->broadcast(Read, address, sender_idx, NotPresent);
        stateOfNewLine = response == HasCopy ? Shared : Exclusive;
    }

    // Insert the new element at the front
    tags.push_front({tag, stateOfNewLine});
    map[tag] = tags.begin();

    return flushed;
}

CacheState LRUSet::write(uint32_t tag, Bus* bus, uint32_t address, int sender_idx) {
    auto map_iter = map.find(tag);

    if (map_iter == map.end()) {
        // tag is not in the set
        return NotPresent;
    }

    auto tags_iter = map_iter->second;

    // send bus signal if state is shared or invalid
    CacheState current_state = tags_iter->second;
    if (current_state == Shared || current_state == Invalid) {
        bus->broadcast(ReadExclusive, address, sender_idx, current_state);
    }

    // set to modified
    tags_iter->second = Modified;

    // move the looked up tag to the front of the tags list
    tags.splice(tags.begin(), tags, tags_iter);
    return current_state;
}

CacheState LRUSet::read(uint32_t tag, Bus* bus, uint32_t address, int sender_idx) {
    auto it = map.find(tag);

    if (it == map.end()) {
        // tag is not in the set
        return NotPresent;
    }

    // send bus signal if state is invalid
    auto tags_iter = it->second;
    CacheState current_state = tags_iter->second;
    if (current_state == Invalid) {
        BusResponse response = bus->broadcast(Read, address, sender_idx, current_state);
        if (response == HasCopy) {
            tags_iter->second = Shared;
        } else if (response == NoResponse) {
            tags_iter->second = Exclusive;
        }
    }

    // move the looked up tag to the front of the tags list
    tags.splice(tags.begin(), tags, it->second);
    return current_state;
}

BusResponse LRUSet::process_signal_from_bus(uint32_t tag, BusMessage message) {
    auto map_iter = map.find(tag);

    if (map_iter == map.end()) {
        // tag is not in set
        return NoResponse;
    }

    // switch state according to protocol
    auto tags_iter = map_iter->second;
    CacheState current_state = tags_iter->second;
    switch (message) {
    case Read:
        if (current_state == Modified) {
            tags_iter->second = Shared;
        } else if (current_state == Exclusive) {
            tags_iter->second = Shared;
        }
        break;
    case Invalidate:
        tags_iter->second = Invalid;
        break;
    case ReadExclusive:
        if (current_state == Modified) {
            tags_iter->second = Invalid;
        } else if (current_state == Exclusive) {
            tags_iter->second = Invalid;
        } else if (current_state == Shared) {
            tags_iter->second = Invalid;
        }
        break;
    case WriteBack:
        break;
    }

    if (current_state == Modified || current_state == Exclusive || current_state == Shared) {
        return HasCopy;
    }
    return NoResponse;
}