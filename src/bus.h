#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <vector>

class Memory;

enum BusMessage {
    ReadExclusive,
    Read,
    WriteBack,
    Invalidate
};

class Bus {
public:
    void broadcast(BusMessage message_type, uint32_t address, int sender_idx);
    long get_total_traffic() const;
    long get_total_invalidations() const;
    void connect_memory(Memory* mem);

    Bus(int _block_size);
private:
    // total traffic from read, read exclusive, write back in bytes
    long long total_traffic;
    long total_invalidations;
    int block_size;

    std::vector<Memory*> memory_blocks;
};

#endif //BUS_H
