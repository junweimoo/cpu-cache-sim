//
// Created by Jun Wei Moo on 4/12/24.
//

#include "profiler.h"

#include "bus.h"

Profiler::Profiler(int num_cores) {
    this->num_cores = num_cores;

    cycles_per_core = std::vector<long long>(num_cores, 0);
    idle_cycles_per_core = std::vector<long long>(num_cores, 0);
    compute_cycles_per_core = std::vector<long long>(num_cores, 0);

    cache_hits_per_core = std::vector<long>(num_cores, 0);
    cache_misses_per_core = std::vector<long>(num_cores, 0);

    loads_per_core = std::vector<long>(num_cores, 0);
    stores_per_core = std::vector<long>(num_cores, 0);
}

void Profiler::update(InstructionType type, int j, int this_cycles, bool is_hit, CacheState from_state, CacheState to_state) {
    switch(type) {
        case LOAD:
            if (is_hit) {
                cache_hits_per_core[j]++;
            } else {
                cache_misses_per_core[j]++;
            }

            loads_per_core[j]++;
            idle_cycles_per_core[j] += this_cycles;

            if (to_state == Modified || to_state == Exclusive || to_state == ExclusiveDragon || to_state == Dirty) {
                private_accesses.fetch_add(1, std::memory_order_relaxed);
            } else if (to_state == Shared || to_state == SharedModified || to_state == SharedClean) {
                shared_accesses.fetch_add(1, std::memory_order_relaxed);
            }
            break;

        case STORE:
            if (is_hit) {
                cache_hits_per_core[j]++;
            } else {
                cache_misses_per_core[j]++;
            }

            stores_per_core[j]++;
            idle_cycles_per_core[j] += this_cycles;

            if (to_state == Modified || to_state == Exclusive || to_state == ExclusiveDragon || to_state == Dirty) {
                private_accesses.fetch_add(1, std::memory_order_relaxed);
            } else if (to_state == Shared || to_state == SharedModified || to_state == SharedClean) {
                shared_accesses.fetch_add(1, std::memory_order_relaxed);
            }
            break;

        case OTHER:
            compute_cycles_per_core[j] += this_cycles;
            break;

        default:
            break;
    }

    cycles_per_core[j] += this_cycles;
}

void Profiler::print_stats(Bus* bus) {
    for (int j = 0; j < num_cores; j++) {
        std::cout << "[Core " << j << "]" << std::endl;
        std::cout << "Cycles: " << cycles_per_core[j] << std::endl;
        std::cout << "Idle cycles: " << idle_cycles_per_core[j] << std::endl;
        std::cout << "Compute cycles: " << compute_cycles_per_core[j] << std::endl;
        std::cout << "Loads: " << loads_per_core[j] << std::endl;
        std::cout << "Stores: " << stores_per_core[j] << std::endl;
        std::cout << "Cache hits: " << cache_hits_per_core[j] << std::endl;
        std::cout << "Cache misses: " << cache_misses_per_core[j] << std::endl;
        std::cout << std::endl;
    }

    std::cout << "[Global]" << std::endl;
    long long max_cycles = cycles_per_core[0];
    for (int j = 1; j < num_cores; j++) {
        max_cycles = std::max(max_cycles, cycles_per_core[j]);
    }
    std::cout << "Overall cycles (maximum among cores): " << max_cycles << std::endl;

    long long total_idle_cycles = 0;
    for (int j = 0; j < num_cores; j++) {
        total_idle_cycles += idle_cycles_per_core[j];
    }
    std::cout << "Total idle cycles: " << total_idle_cycles << std::endl;

    long total_hits = 0;
    long total_misses = 0;
    for (int j = 0; j < num_cores; j++) {
        total_hits += cache_hits_per_core[j];
        total_misses += cache_misses_per_core[j];
    }
    int hit_rate_thousandth = static_cast<float>(total_hits) / (total_hits + total_misses) * 1000;
    std::cout << "Cache hit rate (%): " << hit_rate_thousandth / 10 << "." << hit_rate_thousandth % 10
              << " (" << total_hits << ")" << std::endl;
    int miss_rate_thousandth = 1000 - hit_rate_thousandth;
    std::cout << "Cache miss rate (%): " << miss_rate_thousandth / 10 << "." << miss_rate_thousandth % 10
              << " (" << total_misses << ")" << std::endl;

    std::cout << "Total bus traffic (bytes): " << bus->get_total_traffic() << std::endl;
    std::cout << "Total bus invalidations / updates: " << bus->get_total_invalidations() << std::endl;
    int private_accesses_thousandth = static_cast<float>(private_accesses) / (private_accesses + shared_accesses) * 1000;
    std::cout << "Private data access (%): " << private_accesses_thousandth / 10 << "." << private_accesses_thousandth % 10 << std::endl;
    int shared_accesses_thousandth = 1000 - private_accesses_thousandth;
    std::cout << "Shared data access (%): " << shared_accesses_thousandth / 10 << "." << shared_accesses_thousandth % 10 << std::endl;
}
