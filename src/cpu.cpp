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
    std::cout << "Running CPU simulation..." << std::endl;

    long long total_cycles = 0;
    long long compute_cycles = 0;
    long long idle_cycles = 0;
    long load_store_ins = 0;
    long load_ins = 0;
    long store_ins = 0;
    long cache_hits = 0;
    long cache_misses = 0;

    int num_cores = memories.size();

    std::vector<long long> cycles_per_core(num_cores, 0);
    std::vector<long long> idle_cycles_per_core(num_cores, 0);
    std::vector<long long> compute_cycles_per_core(num_cores, 0);

    std::vector<long> cache_hits_per_core(num_cores, 0);
    std::vector<long> cache_misses_per_core(num_cores, 0);

    std::vector<long> loads_per_core(num_cores, 0);
    std::vector<long> stores_per_core(num_cores, 0);

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
            int this_cycles = 0;
            switch (ins.type) {
            case LOAD:
                p = memories[j]->load(ins.value, bus);

                this_cycles = p.first;

                if (p.second) {
                    cache_hits_per_core[j]++;
                    cache_hits++;
                } else {
                    cache_misses_per_core[j]++;
                    cache_misses++;
                }

                load_store_ins++;

                loads_per_core[j]++;
                load_ins++;

                idle_cycles_per_core[j] += this_cycles;
                idle_cycles += this_cycles;
                break;
            case STORE:
                p = memories[j]->store(ins.value, bus);

                this_cycles = p.first;
                if (p.second) {
                    cache_hits_per_core[j]++;
                    cache_hits++;
                } else {
                    cache_misses_per_core[j]++;
                    cache_misses++;
                }
                load_store_ins++;

                stores_per_core[j]++;
                store_ins++;

                idle_cycles_per_core[j] += this_cycles;
                idle_cycles += this_cycles;
                break;
            case OTHER:
                this_cycles = ins.value;
                compute_cycles_per_core[j] += this_cycles;
                compute_cycles += this_cycles;
                break;
            default:
            }

            cycles_per_core[j] += this_cycles;
            total_cycles += this_cycles;

            i++;

            // std::cout << "cycles: " << std::dec << this_cycles << std::endl;
        }
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

    std::cout << "Total bus traffic (bytes): " << bus->get_total_traffic() << std::endl;
    std::cout << "Total bus invalidations / updates: " << bus->get_total_invalidations() << std::endl;
    std::cout << "Private data access (%): " << std::endl;
    std::cout << "Shared data access (%): " << std::endl;

    // std::cout << "Total cycles: " << total_cycles << std::endl;
    // std::cout << "Compute cycles: " << compute_cycles << std::endl;
    // // std::cout << "Load/store instructions: " << load_store_ins << std::endl;
    // std::cout << "Load instructions: " << load_ins << std::endl;
    // std::cout << "Store instructions: " << store_ins << std::endl;
    // std::cout << "Idle cycles: " << idle_cycles << std::endl;
    // int hit_percent = static_cast<float>(cache_hits) / load_store_ins * 1000;
    // int miss_percent = 1000 - hit_percent;
    // std::cout << "Cache hits: " << cache_hits << " (" << hit_percent / 10 << "." << hit_percent % 10 << "%)" << std::endl;
    // std::cout << "Cache misses: " << cache_misses << " (" << miss_percent / 10 << "." << miss_percent % 10 << "%)" << std::endl;
    // std::cout << "Total bus traffic (bytes): " << bus->get_total_traffic() << std::endl;
    // std::cout << "Total bus invalidations: " << bus->get_total_invalidations() << std::endl;
    // std::cout << "Private data access (%): " <<  << std::endl;
    // std::cout << "Shared data access (%): " <<  << std::endl;
}
