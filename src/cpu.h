#ifndef CPU_H
#define CPU_H

#include <iostream>

class Memory;
class Bus;
class Trace;

class CPU {
public:
    void connectMemory(Memory* mem);
    void connectBus(Bus* bus);
    void run(Trace& trace);

    CPU();
private:
    Memory* memory;
    Bus* bus;
};

#endif
