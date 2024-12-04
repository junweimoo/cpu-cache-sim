#ifndef PROFILER_H
#define PROFILER_H

#include <vector>

#include "enums.h"
#include "trace.h"

class Bus;

class Profiler {
public:
    Profiler(int num_cores);
    void update(InstructionType type, int core_id, int this_cycles, bool is_hit, CacheState from_state, CacheState to_state);
    void print_stats(Bus* bus);

private:
    int num_cores;

    std::vector<long long> cycles_per_core;
    std::vector<long long> idle_cycles_per_core;
    std::vector<long long> compute_cycles_per_core;

    std::vector<long> cache_hits_per_core;
    std::vector<long> cache_misses_per_core;

    std::vector<long> loads_per_core;
    std::vector<long> stores_per_core;

    std::atomic<long> shared_accesses;
    std::atomic<long> private_accesses;
};

#endif //PROFILER_H
