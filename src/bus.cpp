#include "bus.h"
#include "memory.h"

Bus::Bus(int _block_size) : total_invalidations_updates(0), total_traffic(0), block_size(_block_size) {}

BusResponse Bus::broadcast(BusMessage message, uint32_t address, int sender_idx, CacheState sender_cache_state) {
    // std::lock_guard<std::mutex> lock(mtx);

    if (message == WriteBack) {
        // Cache block is written back to memory
        total_traffic++;
        return NoResponse;
    }

    if (message == BusUpdate) {
        // Dragon: Cache block is sent to other caches
        total_traffic++;
    }

    BusResponse orSharedResponses = NoResponse;
    BusResponse orDirtyResponses = NoResponse;
    for (int i = 0; i < memory_blocks.size(); i++) {
        // broadcast to other memory blocks apart from sender
        if (i == sender_idx) continue;
        BusResponse thisResponse = memory_blocks[i]->process_signal_from_bus(message, address, this);

        if (thisResponse == BusResponseShared) {
            // this cache block is also in another clean cache
            orSharedResponses = BusResponseShared;
        }

        if (thisResponse == BusResponseDirty) {
            // this cache block is also in another dirty cache
            orDirtyResponses = BusResponseDirty;
        }

        if (message == BusUpdate) {
            // Dragon: BusUpd updates other copies
            total_invalidations_updates++;
        }

        if (message == ReadExclusive && thisResponse != NoResponse) {
            // MESI: BusReadX invalidates other copies
            total_invalidations_updates++;
        }
    }

    BusResponse finalResponse = orDirtyResponses == BusResponseDirty ? BusResponseDirty :
                                orSharedResponses == BusResponseShared ? BusResponseShared :
                                NoResponse;

    if ((message == ReadExclusive || message == Read) && (finalResponse != NoResponse)) {
        // MESI: cache to cache transfer of cache block
        total_traffic++;
    }

    if (message == ReadDragon && finalResponse != NoResponse) {
        // Dragon: cache to cache transfer of cache block
        total_traffic++;
    }

    return finalResponse;
}

long Bus::get_total_traffic() const {
    return total_traffic * block_size;
    // return total_traffic;
}

long Bus::get_total_invalidations() const {
    return total_invalidations_updates;
}

void Bus::connect_memory(Memory* mem) {
    memory_blocks.push_back(mem);
}
