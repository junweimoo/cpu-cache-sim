#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include "trace.h"

class CPU {
public:
    void connectMemory(Memory* mem);
    void run(Trace& trace);

    CPU();
private:
    Memory* memory;
};

#endif
