// Cache.cpp

#include "Cache.hpp"

#include <fstream>
#include <iomanip>

Cache::Cache(const std::string& filename) : filename_(filename) {}

void Cache::Initialize() {
    cache_lines_.resize(kNumLines, 0x00000000);
    Save();
}

void Cache::WriteLine(uint32_t cache_line, uint32_t data) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    if (cache_line < kNumLines) {
        cache_lines_[cache_line] = data;
    }
    Save();
}

uint32_t Cache::ReadLine(uint32_t cache_line) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    if (cache_line < kNumLines) {
        return cache_lines_[cache_line];
    }
    return 0x00000000;
}

void Cache::InvalidateLine(uint32_t cache_line) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    if (cache_line < kNumLines) {
        cache_lines_[cache_line] = 0x00000000;
    }
    Save();
}

void Cache::Save() {
    std::ofstream file(filename_, std::ios::out | std::ios::trunc);
    for (const auto& word : cache_lines_) {
        file << std::hex << std::setw(8) << std::setfill('0') << word << "\n";
    }
}
