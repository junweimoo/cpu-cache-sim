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
    case ExclusiveDragon:
        return "Exclusive";
    case SharedClean:
        return "SharedClean";
    case SharedModified:
        return "SharedModified";
    case Dirty:
        return "Dirty";
    default:
        return "";
    }
}

LRUSet::LRUSet(int associativity, Protocol _protocol) : max_size(associativity), protocol(_protocol) {
}

CacheState LRUSet::get_state(uint32_t tag) {
    auto map_iter = map.find(tag);
    if (map_iter == map.end()) return NotPresent;
    return map_iter->second->second;
}


std::tuple<bool, BusResponse> LRUSet::allocate(uint32_t tag, bool is_write, Bus* bus, uint32_t address, int sender_idx) {
    auto it = map.find(tag);

    if (it != map.end()) {
        // tag is already in the set
        return {false, NoResponse};
    }

    bool flushed = false;
    if (tags.size() == max_size) {
        // Evict the least recently used element from the back of the tags list
        std::pair<uint32_t, CacheState> p = tags.back();
        int lru_tag = p.first;
        CacheState state = p.second;
        tags.pop_back();
        map.erase(lru_tag);

        // MESI: check if LRU cache needs to be flushe
        if (protocol == MESI) {
            if (state == Modified) {
                flushed = true;
                // Write back flushed element to memory via the bus
                bus->broadcast(WriteBack, address, sender_idx, state);
            }
        }

        // Dragon: check if LRU cache needs to be flushed
        if (protocol == Dragon) {
            if (state == SharedModified || state == Dirty) {
                flushed = true;
                // Write back flushed element to memory via the bus
                bus->broadcast(WriteBack, address, sender_idx, state);
            }
        }
    }

    // send bus signal
    CacheState stateOfNewLine;
    BusResponse response = NoResponse;

    // MESI: broadcast message and set state of new cache line
    if (protocol == MESI) {
        if (is_write) {
            response = bus->broadcast(ReadExclusive, address, sender_idx, NotPresent);
            stateOfNewLine = Modified;
        } else {
            response = bus->broadcast(Read, address, sender_idx, NotPresent);
            stateOfNewLine = response == HasCopy ? Shared : Exclusive;
        }
    }

    // Dragon:
    if (protocol == Dragon) {
        if (is_write) {
            response = bus->broadcast(ReadDragon, address, sender_idx, NotPresent);
            if (response == HasCopy) {
                stateOfNewLine = SharedModified;
                bus->broadcast(BusUpdate, address, sender_idx, SharedModified);
            } else {
                stateOfNewLine = Dirty;
            }
        } else {
            response = bus->broadcast(ReadDragon, address, sender_idx, NotPresent);
            stateOfNewLine = response == HasCopy ? SharedClean : ExclusiveDragon;
        }
    }

    // Insert the new element at the front
    tags.push_front({tag, stateOfNewLine});
    map[tag] = tags.begin();

    return {flushed, response};
}

std::tuple<CacheState, BusResponse, CacheState> LRUSet::write(uint32_t tag, Bus* bus, uint32_t address, int sender_idx) {
    auto map_iter = map.find(tag);

    if (map_iter == map.end()) {
        // tag is not in the set
        return {NotPresent, NoResponse, NotPresent};
    }

    auto tags_iter = map_iter->second;

    // send bus signal if state is shared or invalid
    CacheState current_state = tags_iter->second;
    BusResponse response = NoResponse;

    // MESI: state transition due to write
    if (protocol == MESI) {
        if (current_state == Shared || current_state == Invalid) {
            response = bus->broadcast(ReadExclusive, address, sender_idx, current_state);
        }

        // set to modified
        tags_iter->second = Modified;
    }

    // Dragon:
    if (protocol == Dragon) {
        if (current_state == SharedClean || current_state == SharedModified) {
            response = bus->broadcast(BusUpdate, address, sender_idx, current_state);
            tags_iter->second = response == HasCopy ? SharedModified : Dirty;
        } else if (current_state == ExclusiveDragon) {
            tags_iter->second = Dirty;
        }
    }

    // move the looked up tag to the front of the tags list
    tags.splice(tags.begin(), tags, tags_iter);
    return {current_state, response, tags_iter->second};
}

std::tuple<CacheState, BusResponse, CacheState> LRUSet::read(uint32_t tag, Bus* bus, uint32_t address, int sender_idx) {
    auto it = map.find(tag);

    if (it == map.end()) {
        // tag is not in the set
        return {NotPresent, NoResponse, NotPresent};
    }

    // send bus signal if state is invalid
    auto tags_iter = it->second;
    CacheState current_state = tags_iter->second;
    BusResponse response = NoResponse;

    // MESI: state transition due to read
    if (protocol == MESI) {
        if (current_state == Invalid) {
            response = bus->broadcast(Read, address, sender_idx, current_state);
            if (response == HasCopy) {
                tags_iter->second = Shared;
            } else if (response == NoResponse) {
                tags_iter->second = Exclusive;
            }
        }
    }

    // Dragon: state transition due to read
    if (protocol == Dragon) {
        // No state transitions in all cases
    }

    // move the looked up tag to the front of the tags list
    tags.splice(tags.begin(), tags, it->second);
    return {current_state, response, tags_iter->second};
}

BusResponse LRUSet::process_signal_from_bus(uint32_t tag, BusMessage message, Bus* bus, uint32_t address, int sender_idx) {
    auto map_iter = map.find(tag);

    if (map_iter == map.end()) {
        // tag is not in set
        return NoResponse;
    }

    // switch state according to protocol
    auto tags_iter = map_iter->second;
    CacheState current_state = tags_iter->second;

    switch (message) {
        // MESI signals
        case Read:
            if (current_state == Modified) {
                tags_iter->second = Shared;
                bus->broadcast(WriteBack, address, sender_idx, Shared);
            } else if (current_state == Exclusive) {
                tags_iter->second = Shared;
            }
            break;
        case ReadExclusive:
            if (current_state == Modified) {
                tags_iter->second = Invalid;
                bus->broadcast(WriteBack, address, sender_idx, Invalid);
            } else if (current_state == Exclusive) {
                tags_iter->second = Invalid;
            } else if (current_state == Shared) {
                tags_iter->second = Invalid;
            }
            break;

        // Dragon signals
        case ReadDragon:
            if (current_state == ExclusiveDragon) {
                tags_iter->second = SharedClean;
            } else if (current_state == Dirty) {
                tags_iter->second = SharedModified;
                // Flush
                bus->broadcast(WriteBack, address, sender_idx, SharedModified);
            } else if (current_state == SharedModified) {
                // Flush
                bus->broadcast(WriteBack, address, sender_idx, SharedModified);
            }
            break;
        case BusUpdate:
            if (current_state == SharedModified) {
                tags_iter->second = SharedClean;
            }
            break;

        // Common signals
        case WriteBack:
        default:
            break;
    }

    if (current_state == Modified || current_state == Exclusive || current_state == Shared
            || current_state == SharedClean || current_state == SharedModified || current_state == ExclusiveDragon
            || current_state == Dirty) {
        return HasCopy;
    }
    return NoResponse;
}