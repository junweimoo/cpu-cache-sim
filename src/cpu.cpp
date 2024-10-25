#include "cpu.h"
#include <iostream>

CPU::CPU() {}

void CPU::connectMemory(Memory* mem) {
    memory = mem;
}

void CPU::run(Trace& trace) {
    std::cout << "Running CPU simulation..." << std::endl;

    long long total_cycles = 0;
    long long compute_cycles = 0;
    long long idle_cycles = 0;
    long load_store_ins = 0;
    long cache_hits = 0;
    long cache_misses = 0;

    int i = 0;
    while (i < 10 && trace.has_next_instruction()) {
        const Instruction& ins = trace.get_current_instruction();

        std::cout << ins.type << " " << ins.value << std::endl;

        std::pair<int, bool> p;
        int this_cycles = 0;
        switch (ins.type) {
        case LOAD:
            p = memory->load(ins.value);
            this_cycles = p.first;
            if (p.second) {
                cache_hits++;
            } else {
                cache_misses++;
            }
            load_store_ins++;
            idle_cycles += this_cycles;
            break;
        case STORE:
            p = memory->store(ins.value);
            this_cycles = p.first;
            if (p.second) {
                cache_hits++;
            } else {
                cache_misses++;
            }
            load_store_ins++;
            idle_cycles += this_cycles;
            break;
        case OTHER:
            this_cycles = ins.value;
            compute_cycles += this_cycles;
            break;
        default:
        }

        total_cycles += this_cycles;
        std::cout << "cycles: " << this_cycles << std::endl;

        i++;
    }

    std::cout << "Total cycles: " << total_cycles << std::endl;
    std::cout << "Compute cycles: " << compute_cycles << std::endl;
    std::cout << "Load/store instruction: " << load_store_ins << std::endl;
    std::cout << "Idle cycles: " << idle_cycles << std::endl;
    std::cout << "Cache hits: " << cache_hits << std::endl;
    std::cout << "Cache misses: " << cache_misses << std::endl;
    // std::cout << "Total bus traffic (bytes): " << std::endl;
    // std::cout << "Total bus updates: " << std::endl;
    // std::cout << "Private data access (%): " << std::endl;
    // std::cout << "Shared data access (%): " << std::endl;
}
