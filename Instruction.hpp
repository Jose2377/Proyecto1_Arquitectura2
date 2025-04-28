// Instruction.hpp

#ifndef INSTRUCTION_HPP_
#define INSTRUCTION_HPP_

#include <string>
#include <cstdint>

struct Instruction {
    std::string message_type;
    uint8_t src = 0;
    uint8_t dest = 0;
    uint32_t addr = 0;
    uint32_t size = 0;
    uint32_t start_cache_line = 0;
    uint32_t num_cache_lines = 0;
    uint8_t qos = 0;
    uint32_t data = 0;   // Para READ_RESP
    uint8_t status = 0;  // Para WRITE_RESP (0x1 = OK, 0x0 = NOT_OK)
};

#endif  // INSTRUCTION_HPP_
