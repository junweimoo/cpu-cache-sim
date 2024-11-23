#include "cpu.h"

#include "memory.h"
#include "trace.h"
#include "bus.h"

CPU::CPU() {}

CPU::~CPU() {
    for (Memory* memory : memories) delete memory;
    for (Trace* trace : traces) delete trace;
}

void CPU::add_core(Trace *trace, Memory *memory) {
    traces.push_back(trace);
    memories.push_back(memory);
}

void CPU::connect_bus(Bus *_bus) {
    bus = _bus;
}

void CPU::run() {
    bool is_debug = true;

    std::cout << "Running CPU simulation..." << std::endl;

    const size_t num_cores = memories.size();

    std::vector<long long> cycles_per_core(num_cores, 0);
    std::vector<long long> idle_cycles_per_core(num_cores, 0);
    std::vector<long long> compute_cycles_per_core(num_cores, 0);

    std::vector<long> cache_hits_per_core(num_cores, 0);
    std::vector<long> cache_misses_per_core(num_cores, 0);

    std::vector<long> loads_per_core(num_cores, 0);
    std::vector<long> stores_per_core(num_cores, 0);

    long shared_accesses = 0;
    long private_accesses = 0;

    auto start = std::chrono::high_resolution_clock::now();

    int i = 0;
    bool is_over = false;

    while (!is_over) {
        is_over = true;
        for (int j = 0; j < memories.size(); j++) {
            if (!traces[j]->has_next_instruction()) continue;

            is_over = false;

            const Instruction& ins = traces[j]->get_current_instruction();

            // std::cout << ins.type << " " << std::hex << ins.value << std::endl;

            std::pair<int, bool> p;

            int this_cycles;
            bool is_hit;
            CacheState from_state, to_state;

            switch (ins.type) {
                case LOAD:
                std::tie(this_cycles, is_hit, from_state, to_state) = memories[j]->load(ins.value, bus);

                if (is_hit) {
                    cache_hits_per_core[j]++;
                } else {
                    cache_misses_per_core[j]++;
                }

                loads_per_core[j]++;

                idle_cycles_per_core[j] += this_cycles;

                if (to_state == Modified || to_state == Exclusive || to_state == ExclusiveDragon || to_state == Dirty) {
                    private_accesses++;
                } else if (to_state == Shared || to_state == SharedModified || to_state == SharedClean) {
                    shared_accesses++;
                }

                if (is_debug)
                    std::cout << "core" << j <<  " " << i << " [load] from_state:" << LRUSet::get_cache_state_str(from_state)
                              << " to_state:" << LRUSet::get_cache_state_str(to_state) << std::endl;
                break;
            case STORE:
                std::tie(this_cycles, is_hit, from_state, to_state) = memories[j]->store(ins.value, bus);

                if (is_hit) {
                    cache_hits_per_core[j]++;
                } else {
                    cache_misses_per_core[j]++;
                }

                stores_per_core[j]++;

                idle_cycles_per_core[j] += this_cycles;

                if (to_state == Modified || to_state == Exclusive || to_state == ExclusiveDragon || to_state == Dirty) {
                    private_accesses++;
                } else if (to_state == Shared || to_state == SharedModified || to_state == SharedClean) {
                    shared_accesses++;
                }

                if (is_debug)
                    std::cout << "core" << j <<  " " << i << " [store] from_state:" << LRUSet::get_cache_state_str(from_state)
                              << " to_state:" << LRUSet::get_cache_state_str(to_state) << std::endl;
                break;
            case OTHER:
                this_cycles = ins.value;
                compute_cycles_per_core[j] += this_cycles;
                break;
            default:
                break;
            }

            cycles_per_core[j] += this_cycles;

            if (is_debug) std::cout << "cycles: " << std::dec << this_cycles << std::endl;
        }
        i++;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Simulation finished! (" << duration.count() << "ms)" << std::endl << std::endl;

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
    std::cout << "Cache hit rate (%): " << hit_rate_thousandth / 10 << "." << hit_rate_thousandth % 10 << std::endl;
    int miss_rate_thousandth = 1000 - hit_rate_thousandth;
    std::cout << "Cache miss rate (%): " << miss_rate_thousandth / 10 << "." << miss_rate_thousandth % 10 << std::endl;

    std::cout << "Total bus traffic (bytes): " << bus->get_total_traffic() << std::endl;
    std::cout << "Total bus invalidations / updates: " << bus->get_total_invalidations() << std::endl;
    int private_accesses_thousandth = static_cast<float>(private_accesses) / (private_accesses + shared_accesses) * 1000;
    std::cout << "Private data access (%): " << private_accesses_thousandth / 10 << "." << private_accesses_thousandth % 10 << std::endl;
    int shared_accesses_thousandth = 1000 - private_accesses_thousandth;
    std::cout << "Shared data access (%): " << shared_accesses_thousandth / 10 << "." << shared_accesses_thousandth % 10 << std::endl;
}
