// Memory.cpp

#include "Memory.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>      // << añadelo

Memory::Memory(const std::string& filename) : filename_(filename) {}

void Memory::Initialize() {
    mem_.resize(4096, 0x00000000);  // 4096 x 32 bits
    Save();
}

void Memory::Write(uint32_t addr, uint32_t data) {
    std::lock_guard<std::mutex> lock(mem_mutex_);
    if (addr / 4 >= mem_.size()) return;
    mem_[addr / 4] = data;

    // --- LOG: imprimir dirección y dato ---
    std::cout
    << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr
    << ": "
    << std::hex << std::setw(8) << std::setfill('0') << data
    << std::dec  // volver a decimal para futuros cout
    << std::endl;

    Save();  // persiste todo el bloque de memoria en el fichero :contentReference[oaicite:0]{index=0}&#8203;:contentReference[oaicite:1]{index=1}
}

uint32_t Memory::Read(uint32_t addr) {
    std::lock_guard<std::mutex> lock(mem_mutex_);
    if (addr / 4 >= mem_.size()) return 0;
    Save();
    return mem_[addr / 4];
}

void Memory::Save() {
    std::ofstream file(filename_, std::ios::out | std::ios::trunc);
    for (size_t i = 0; i < mem_.size(); i += 4) {
        uint32_t addr = static_cast<uint32_t>(i * 4);
        file << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr << ": ";
        for (size_t j = 0; j < 4 && (i + j) < mem_.size(); ++j) {
            file << std::hex << std::setw(8) << std::setfill('0') << mem_[i + j];
            if (j < 3 && (i + j + 1) < mem_.size()) file << " ";
        }
        file << "\n";
    }
}
