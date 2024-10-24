#ifndef MEMORY_H
#define MEMORY_H

class Memory {
public:
    Memory();
    int load(int address);
    void store(int address, int value);
};

#endif
