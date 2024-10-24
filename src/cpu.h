#ifndef CPU_H
#define CPU_H

#include "memory.h"

class CPU {
public:
    void connectMemory(Memory* mem);
    void run();

private:
    Memory* memory;
};

#endif
