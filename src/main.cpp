#include "cpu.h"
#include "trace.h"
#include "memory.h"
#include "bus.h"
#include "config.h"

#define NUM_PROCESSORS 1

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

    Bus bus(block_size);

    CPU cpu;
    cpu.connectBus(&bus);

    for (int i = 0; i < NUM_PROCESSORS; i++) {
        // read data from file
        Trace* trace = new Trace();
        if (!trace->read_data(filename)) {
            return EXIT_FAILURE;
        }

        // set up memory
        Memory* memory = new Memory(0, cache_size, associativity, block_size, Config::ADDRESS_BITS);
        bus.connect_memory(memory);

        cpu.add_core(trace, memory);
    }

    // simulate
    cpu.run();

    return 0;
}
