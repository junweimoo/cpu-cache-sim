#include "cpu.h"

#include "memory.h"
#include "trace.h"
#include "bus.h"

CPU::CPU() {}

void CPU::connectMemory(Memory* mem) {
    memory = mem;
}

void CPU::connectBus(Bus *_bus) {
    bus = _bus;
}

void CPU::run(Trace& trace) {
    std::cout << "Running CPU simulation..." << std::endl;

    long long total_cycles = 0;
    long long compute_cycles = 0;
    long long idle_cycles = 0;
    long load_store_ins = 0;
    long load_ins = 0;
    long store_ins = 0;
    long cache_hits = 0;
    long cache_misses = 0;

    auto start = std::chrono::high_resolution_clock::now();

    int i = 0;
    while (trace.has_next_instruction()) {
        const Instruction& ins = trace.get_current_instruction();

        // std::cout << ins.type << " " << std::hex << ins.value << std::endl;

        std::pair<int, bool> p;
        int this_cycles = 0;
        switch (ins.type) {
        case LOAD:
            p = memory->load(ins.value, bus);
            this_cycles = p.first;
            if (p.second) {
                cache_hits++;
            } else {
                cache_misses++;
            }
            load_store_ins++;
            load_ins++;
            idle_cycles += this_cycles;
            break;
        case STORE:
            p = memory->store(ins.value, bus);
            this_cycles = p.first;
            if (p.second) {
                cache_hits++;
            } else {
                cache_misses++;
            }
            load_store_ins++;
            store_ins++;
            idle_cycles += this_cycles;
            break;
        case OTHER:
            this_cycles = ins.value;
            compute_cycles += this_cycles;
            break;
        default:
        }

        total_cycles += this_cycles;
        i++;

        // std::cout << "cycles: " << std::dec << this_cycles << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Simulation finished! (" << duration.count() << "ms)" << std::endl << std::endl;


    std::cout << "Total cycles: " << total_cycles << std::endl;
    std::cout << "Compute cycles: " << compute_cycles << std::endl;
    // std::cout << "Load/store instructions: " << load_store_ins << std::endl;
    std::cout << "Load instructions: " << load_ins << std::endl;
    std::cout << "Store instructions: " << store_ins << std::endl;
    std::cout << "Idle cycles: " << idle_cycles << std::endl;
    int hit_percent = static_cast<float>(cache_hits) / load_store_ins * 1000;
    int miss_percent = 1000 - hit_percent;
    std::cout << "Cache hits: " << cache_hits << " (" << hit_percent / 10 << "." << hit_percent % 10 << "%)" << std::endl;
    std::cout << "Cache misses: " << cache_misses << " (" << miss_percent / 10 << "." << miss_percent % 10 << "%)" << std::endl;
    std::cout << "Total bus traffic (bytes): " << bus->get_total_traffic() << std::endl;
    std::cout << "Total bus invalidations: " << bus->get_total_invalidations() << std::endl;
    // std::cout << "Private data access (%): " <<  << std::endl;
    // std::cout << "Shared data access (%): " <<  << std::endl;
}
