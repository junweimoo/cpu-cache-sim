#ifndef TRACE_H
#define TRACE_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

enum InstructionType {
    LOAD,
    STORE,
    OTHER
};

struct Instruction {
    InstructionType type;
    int value;
};

class Trace {
public:
    Trace();
    bool read_data(const std::string& filename);

    const Instruction& get_current_instruction();
    bool has_next_instruction() const;

private:
    std::vector<Instruction> data;
    int current_instruction;
};

#endif // TRACE_H
