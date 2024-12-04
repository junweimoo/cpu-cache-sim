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
    std::lock_guard<std::mutex> lock(mtx);
    auto map_iter = map.find(tag);
    if (map_iter == map.end()) return NotPresent;
    return map_iter->second->second;
}

BusResponse LRUSet::unlock_and_broadcast(std::unique_lock<std::mutex>& lock, Bus* bus, BusMessage message, uint32_t address, int sender_idx, CacheState sender_cache_state) {
    lock.unlock();
    BusResponse res = bus->broadcast(message, address, sender_idx, sender_cache_state);
    lock.lock();
    return res;
}

std::tuple<bool, BusResponse> LRUSet::allocate(uint32_t tag, bool is_write, Bus* bus, uint32_t address, int sender_idx) {
    std::unique_lock<std::mutex> lock(mtx);
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

        // MESI: check if LRU cache needs to be flushed
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
            response = unlock_and_broadcast(lock, bus, ReadExclusive, address, sender_idx, NotPresent);
            stateOfNewLine = Modified;
        } else {
            response = unlock_and_broadcast(lock, bus, Read, address, sender_idx, NotPresent);
            if (response == BusResponseShared || response == BusResponseDirty) {
                // Other copies present -> new cache block is Shared
                stateOfNewLine = Shared;
            } else if (response == NoResponse) {
                // No other copies -> new cache block is Exclusive
                stateOfNewLine = Exclusive;
            }
        }
    }

    // Dragon: broadcast message and set state of new cache line
    if (protocol == Dragon) {
        if (is_write) {
            response = unlock_and_broadcast(lock, bus, ReadDragon, address, sender_idx, NotPresent);
            if (response == BusResponseShared || response == BusResponseDirty) {
                unlock_and_broadcast(lock, bus, BusUpdate, address, sender_idx, NotPresent);
                stateOfNewLine = SharedModified;
            } else {
                stateOfNewLine = Dirty;
            }
        } else {
            response = unlock_and_broadcast(lock, bus, ReadDragon, address, sender_idx, NotPresent);
            if (response == BusResponseShared || response == BusResponseDirty) {
                // Other copies present -> new cache block is SharedClean
                stateOfNewLine = SharedClean;
            } else {
                // No other copies -> new Cache block is Exclusive
                stateOfNewLine = ExclusiveDragon;
            }
        }
    }

    // Insert the new element at the front
    tags.push_front({tag, stateOfNewLine});
    map[tag] = tags.begin();

    return {flushed, response};
}

std::tuple<CacheState, BusResponse, CacheState> LRUSet::write(uint32_t tag, Bus* bus, uint32_t address, int sender_idx) {
    std::unique_lock<std::mutex> lock(mtx);
    auto map_iter = map.find(tag);

    if (map_iter == map.end()) {
        // tag is not in the set
        return {NotPresent, NoResponse, NotPresent};
    }

    auto tags_iter = map_iter->second;
    CacheState current_state = tags_iter->second;
    BusResponse response = NoResponse;

    // MESI: state transition due to write
    if (protocol == MESI) {
        // Send BusRdX if state is Shared or Invalid
        if (current_state == Shared || current_state == Invalid) {
            response =  unlock_and_broadcast(lock, bus, ReadExclusive, address, sender_idx, current_state);
        }
        tags_iter->second = Modified;
    }

    // Dragon: state transition due to write
    if (protocol == Dragon) {
        // Send BusUpd if state is Sc or Sm
        if (current_state == SharedClean || current_state == SharedModified) {
            response = unlock_and_broadcast(lock, bus, BusUpdate, address, sender_idx, current_state);
            if (response == BusResponseShared || response == BusResponseDirty) {
                // Another cache with Sc / Sm -> transition to SharedModified
                tags_iter->second = SharedModified;
            } else if (response == NoResponse) {
                // No other copies -> transition to Dirty
                tags_iter->second = Dirty;
            }
        } else if (current_state == ExclusiveDragon) {
            tags_iter->second = Dirty;
        }
    }

    // move the looked up tag to the front of the tags list
    tags.splice(tags.begin(), tags, tags_iter);
    return {current_state, response, tags_iter->second};
}

std::tuple<CacheState, BusResponse, CacheState> LRUSet::read(uint32_t tag, Bus* bus, uint32_t address, int sender_idx) {
    std::unique_lock<std::mutex> lock(mtx);
    auto it = map.find(tag);

    if (it == map.end()) {
        // tag is not in the set
        return {NotPresent, NoResponse, NotPresent};
    }

    auto tags_iter = it->second;
    CacheState current_state = tags_iter->second;
    BusResponse response = NoResponse;

    // MESI: state transition due to read
    if (protocol == MESI) {
        // Send BusRd if state is Invalid
        if (current_state == Invalid) {
            response = unlock_and_broadcast(lock, bus, Read, address, sender_idx, current_state);
            if (response == BusResponseShared || response == BusResponseDirty) {
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
    std::unique_lock<std::mutex> lock(mtx);
    auto map_iter = map.find(tag);

    if (map_iter == map.end()) {
        // tag is not in set
        return NoResponse;
    }

    // process bus message according to protocol
    auto tags_iter = map_iter->second;
    CacheState current_state = tags_iter->second;

    switch (message) {
        // MESI signals
        case Read:
            if (current_state == Modified) {
                bus->broadcast(WriteBack, address, sender_idx, current_state);
                tags_iter->second = Shared;
                return BusResponseDirty;
            }
            if (current_state == Exclusive) {
                tags_iter->second = Shared;
                return BusResponseShared;
            }
            if (current_state == Shared) {
                return BusResponseShared;
            }
            break;
        case ReadExclusive:
            if (current_state == Modified) {
                bus->broadcast(WriteBack, address, sender_idx, current_state);
                tags_iter->second = Invalid;
                return BusResponseDirty;
            }
            if (current_state == Exclusive) {
                tags_iter->second = Invalid;
                return BusResponseShared;
            }
            if (current_state == Shared) {
                tags_iter->second = Invalid;
                return BusResponseShared;
            }
            break;

        // Dragon signals
        case ReadDragon:
            if (current_state == ExclusiveDragon) {
                tags_iter->second = SharedClean;
                return BusResponseShared;
            }
            if (current_state == Dirty) {
                bus->broadcast(WriteBack, address, sender_idx, current_state);
                tags_iter->second = SharedModified;
                return BusResponseDirty;
            }
            if (current_state == SharedClean) {
                return BusResponseShared;
            }
            if (current_state == SharedModified) {
                bus->broadcast(WriteBack, address, sender_idx, current_state);
                return BusResponseDirty;
            }
            break;
        case BusUpdate:
            if (current_state == SharedClean) {
                return BusResponseShared;
            }
            if (current_state == SharedModified) {
                tags_iter->second = SharedClean;
                return BusResponseDirty;
            }
            break;

        // Common signals
        case WriteBack:
        default:
            break;
    }

    return NoResponse;
}