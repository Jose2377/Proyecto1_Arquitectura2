// Instruction.hpp

#ifndef INSTRUCTION_HPP_
#define INSTRUCTION_HPP_

#include <string>
#include <cstdint>

struct Instruction {
    std::string message_type;
    uint8_t src;
    uint32_t addr;
    uint32_t size;
    uint8_t qos;
};

#endif  // INSTRUCTION_HPP_
