#include "cpu.h"
#include "memory.h"
#include <iostream>

int main() {
    CPU cpu;
    Memory memory;

    cpu.connectMemory(&memory);
    cpu.run();

    std::cout << "Simulation finished!" << std::endl;
    return 0;
}
