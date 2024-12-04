#include <thread>

#include "cpu.h"
#include "memory.h"
#include "trace.h"
#include "bus.h"
#include "profiler.h"

#define is_debug false

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

void CPU::run_core(int j, Profiler& profiler) {
    int i = 0;
    while (traces[j]->has_next_instruction()) {
        const Instruction& ins = traces[j]->get_current_instruction();

        int this_cycles;
        bool is_hit;
        CacheState from_state, to_state;

        switch (ins.type) {
            case LOAD:
                std::tie(this_cycles, is_hit, from_state, to_state) = memories[j]->load(ins.value, bus);
                profiler.update(LOAD, j, this_cycles, is_hit, from_state, to_state);
                break;
            case STORE:
                std::tie(this_cycles, is_hit, from_state, to_state) = memories[j]->store(ins.value, bus);
                profiler.update(STORE, j, this_cycles, is_hit, from_state, to_state);
                break;
            case OTHER:
                profiler.update(STORE, j, this_cycles, is_hit, from_state, to_state);
                break;
            default:
                break;
        }
        i++;
    }
}

void CPU::run_parallel() {
    std::cout << "Running CPU simulation..." << std::endl;

    const size_t num_cores = memories.size();
    std::vector<std::thread> threads;
    Profiler profiler(num_cores);

    auto start = std::chrono::high_resolution_clock::now();

    threads.reserve(num_cores);
    for (size_t i = 0; i < num_cores; i++) {
        threads.emplace_back([this, i, &profiler]() {
            this->run_core(i, profiler);
        });
    }

    for (size_t i = 0; i < num_cores; i++) {
        threads[i].join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Simulation finished! (" << duration.count() << "ms)" << std::endl << std::endl;

    profiler.print_stats(bus);
}


void CPU::run_serial() {
    std::cout << "Running CPU simulation..." << std::endl;

    const size_t num_cores = memories.size();

    Profiler profiler(num_cores);

    auto start = std::chrono::high_resolution_clock::now();

    int i = 0;
    bool is_over = false;

    while (!is_over) {
        is_over = true;
        for (int j = 0; j < memories.size(); j++) {
            if (!traces[j]->has_next_instruction()) continue;

            is_over = false;

            const Instruction& ins = traces[j]->get_current_instruction();

            if constexpr (is_debug) std::cout << ins.type << " " << std::hex << ins.value << std::endl;

            std::pair<int, bool> p;

            int this_cycles;
            bool is_hit;
            CacheState from_state, to_state;

            int prev_traffic = bus->get_total_traffic();
            int prev_invalidations = bus->get_total_invalidations();

            switch (ins.type) {
                case LOAD:
                std::tie(this_cycles, is_hit, from_state, to_state) = memories[j]->load(ins.value, bus);

                profiler.update(LOAD, j, this_cycles, is_hit, from_state, to_state);

                if constexpr (is_debug) std::cout << "core" << j <<  " " << i << " [load] from_state:"
                                        << LRUSet::get_cache_state_str(from_state)
                                        << " to_state:" << LRUSet::get_cache_state_str(to_state) << std::endl;
                break;
            case STORE:
                std::tie(this_cycles, is_hit, from_state, to_state) = memories[j]->store(ins.value, bus);

                profiler.update(STORE, j, this_cycles, is_hit, from_state, to_state);

                if constexpr (is_debug) std::cout << "core" << j <<  " " << i << " [store] from_state:"
                                        << LRUSet::get_cache_state_str(from_state)
                                        << " to_state:" << LRUSet::get_cache_state_str(to_state) << std::endl;
                break;
            case OTHER:
                profiler.update(STORE, j, this_cycles, is_hit, from_state, to_state);
                break;
            default:
                break;
            }

            if constexpr (is_debug) std::cout << "cycles: " << std::dec << this_cycles
                                    << " traffic: " << bus->get_total_traffic() - prev_traffic
                                    << " invalidations/updates: " << bus->get_total_invalidations() << std::endl;
        }
        i++;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Simulation finished! (" << duration.count() << "ms)" << std::endl << std::endl;

    profiler.print_stats(bus);
}
