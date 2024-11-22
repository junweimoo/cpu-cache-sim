#include "trace.h"

Trace::Trace() : current_instruction(0) {}

bool Trace::read_data(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Error: Unable to open file '" << filename << "'." << std::endl;
        return false;
    }

    std::cout << "Reading data from file '" << filename << "'." << std::endl;
    std::string line;
    int line_number = 0;
    while (std::getline(infile, line)) {
        line_number++;
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string type_str;
        std::string value_str;

        if (!(iss >> type_str >> value_str)) {
            std::cerr << "Error: Invalid format at line " << line_number << ": '"
                      << line << "'. Skipping." << std::endl;
            continue;
        }

        if (type_str == "//") {
            continue;
        }

        int type_int = std::stoi(type_str);
        InstructionType type;
        switch (type_int) {
        case 0:
            type = LOAD;
            break;
        case 1:
            type = STORE;
            break;
        case 2:
            type = OTHER;
            break;
        default:
            std::cerr << "Warning: Unknown instruction type " << type_int
                      << " at line " << line_number << ". Skipping." << std::endl;
            continue;
        }

        int value;
        try {
            if (value_str.find("0x") == 0) {
                value = std::stoi(value_str.substr(2), nullptr, 16);
            } else {
                value = std::stoi(value_str, nullptr, 16);
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error: Invalid hexadecimal value '" << value_str
                      << "' at line " << line_number << ". Skipping." << std::endl;
            continue;
        } catch (const std::out_of_range& e) {
            std::cerr << "Error: Hexadecimal value out of range '" << value_str
                      << "' at line " << line_number << ". Skipping." << std::endl;
            continue;
        }

        data.emplace_back(Instruction{type, value});
    }

    infile.close();

    if (data.empty()) {
        std::cerr << "Warning: No valid instructions loaded from '" << filename << "'." << std::endl;
        return false;
    }

    std::cout << "Successfully loaded " << data.size() << " instructions from '" << filename << "'." << std::endl;
    return true;
}

const Instruction& Trace::get_current_instruction() {
    if (current_instruction < data.size()) {
        return data[current_instruction++];
    } else {
        throw std::out_of_range("No further instructions available.");
    }
}

bool Trace::has_next_instruction() const {
    return current_instruction < data.size();
}
