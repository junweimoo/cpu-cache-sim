#ifndef CPU_H
#define CPU_H

#include <iostream>

class Profiler;
class Memory;
class Bus;
class Trace;

class CPU {
public:
    void connect_bus(Bus* bus);
    void run_serial();
    void run_parallel();
    void add_core(Trace* trace, Memory* memory);

    CPU();
    ~CPU();
private:
    void run_core(int code_id, Profiler& profiler);
    std::vector<Trace*> traces;
    std::vector<Memory*> memories;
    Bus* bus;
};

#endif
