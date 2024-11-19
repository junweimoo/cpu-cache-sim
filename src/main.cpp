#include "cpu.h"
#include "trace.h"
#include "memory.h"
#include "bus.h"
#include "config.h"

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <protocol> <filename> <cache_size> <associativity> <block_size>" << std::endl;
        return EXIT_FAILURE;
    }

    // arguments
    std::string protocol = argv[1];
    std::string filename = argv[2];
    int cache_size = atoi(argv[3]);
    int associativity = atoi(argv[4]);
    int block_size = atoi(argv[5]);

    // read data from file
    Trace trace;
    if (!trace.read_data(filename)) {
        return EXIT_FAILURE;
    }

    // set up system
    Memory memory(cache_size, associativity, block_size, Config::ADDRESS_BITS);

    Bus bus(block_size);
    bus.connect_memory(&memory);

    CPU cpu;
    cpu.connectMemory(&memory);
    cpu.connectBus(&bus);
    cpu.run(trace);

    return 0;
}
