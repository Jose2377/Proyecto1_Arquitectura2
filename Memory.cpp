// Memory.cpp

#include "Memory.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>

Memory::Memory(const std::string& filename) : filename_(filename) {}

void Memory::Initialize() {
    mem_.resize(4096, 0x00000000);  // 4096 x 32 bits, inicializados a 0
    Save();
}

void Memory::Write(uint32_t addr, uint32_t data) {
    std::lock_guard<std::mutex> lock(mem_mutex_);
    if (addr / 4 >= mem_.size()) return;
    mem_[addr / 4] = data;
    Save();
}

uint32_t Memory::Read(uint32_t addr) {
    std::lock_guard<std::mutex> lock(mem_mutex_);
    if (addr / 4 >= mem_.size()) return 0;
    return mem_[addr / 4];
}

void Memory::Save() {
    std::ofstream file(filename_, std::ios::out | std::ios::trunc);
    for (const auto& word : mem_) {
        file << std::hex << std::setw(8) << std::setfill('0') << word << "\n";
    }
}
