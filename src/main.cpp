#include "cpu.h"
#include "trace.h"
#include "memory.h"
#include "bus.h"
#include "config.h"

#define NUM_PROCESSORS 4

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <protocol> <filename> <cache_size> <associativity> <block_size>" << std::endl;
        return EXIT_FAILURE;
    }

    // arguments
    Protocol protocol = std::strcmp(argv[1], "Dragon") == 0 ? Dragon : MESI;
    std::string filename = argv[2];
    int cache_size = atoi(argv[3]);
    int associativity = atoi(argv[4]);
    int block_size = atoi(argv[5]);

    Bus bus(block_size);

    CPU cpu;
    cpu.connect_bus(&bus);

    for (int i = 0; i < NUM_PROCESSORS; i++) {
        std::string core_filename = filename + "_" + std::to_string(i) + ".data";
        if (!std::filesystem::exists(core_filename)) break;
        // read data from file
        Trace* trace = new Trace();
        if (!trace->read_data(core_filename)) {
            return EXIT_FAILURE;
        }

        // set up memory
        Memory* memory = new Memory(i, cache_size, associativity, block_size, Config::ADDRESS_BITS, protocol);
        bus.connect_memory(memory);

        cpu.add_core(trace, memory);
        std::cout << std::endl;
    }

    // simulate
    std::cout << "Protocol: " << (protocol == Dragon ? "Dragon" : "MESI") <<  std::endl;
    cpu.run_parallel();

    return 0;
}
