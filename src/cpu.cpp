#include "cpu.h"
#include <iostream>

void CPU::connectMemory(Memory* mem) {
    memory = mem;
}

void CPU::run() {
    std::cout << "Running CPU simulation..." << std::endl;
}
