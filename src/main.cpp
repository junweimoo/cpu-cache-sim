#include "cpu.h"
#include "memory.h"
#include "trace.h"
#include "config.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <protocol> <filename> <cache_size> <associativity> <block_size>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string protocol = argv[1];
    std::string filename = argv[2];
    int cache_size = atoi(argv[3]);
    int associativity = atoi(argv[4]);
    int block_size = atoi(argv[5]);

    Trace trace;
    if (!trace.read_data(filename)) {
        return EXIT_FAILURE;
    }

    Memory memory(cache_size, associativity, block_size, Config::ADDRESS_BITS);
    CPU cpu;

    cpu.connectMemory(&memory);
    cpu.run(trace);

    std::cout << "Simulation finished!" << std::endl;
    return 0;
}
