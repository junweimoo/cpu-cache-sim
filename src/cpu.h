#ifndef CPU_H
#define CPU_H

#include <iostream>

class Memory;
class Bus;
class Trace;

class CPU {
public:
    void connectBus(Bus* bus);
    void run();
    void add_core(Trace* trace, Memory* memory);

    CPU();
    ~CPU();
private:
    std::vector<Trace*> traces;
    std::vector<Memory*> memories;
    Bus* bus;
};

#endif
