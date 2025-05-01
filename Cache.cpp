// Cache.cpp

#include "Cache.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>


Cache::Cache(const std::string& filename) : filename_(filename) {}

void Cache::Initialize() {
    cache_lines_.resize(kNumLines, 0x00000000);
    Save();
}

void Cache::WriteLine(uint32_t cache_line, uint32_t data) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    if (cache_line < kNumLines) {
        cache_lines_[cache_line] = data;

        // --- LOG: imprimir dirección y dato ---
        // asumimos que cada línea de cache mapea 4 bytes consecutivos:
        uint32_t addr = cache_line * 4;
        std::cout
        << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr
        << ": "
        << std::hex << std::setw(8) << std::setfill('0') << data
        << std::dec
        << std::endl;
    }
    Save();  // persiste todo el contenido de cache en el fichero :contentReference[oaicite:2]{index=2}&#8203;:contentReference[oaicite:3]{index=3}
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
    // Cada línea mostrará 4 palabras (4×4 B = 16 Bytes)
    for (size_t i = 0; i < cache_lines_.size(); i += 4) {
        // calcula la dirección base de este grupo de 4 palabras
        uint32_t addr = static_cast<uint32_t>(i * 4);

        // imprime la dirección en hex (8 dígitos) seguida de “: ”
        file << "0x"
        << std::hex << std::setw(8) << std::setfill('0') << addr
        << ": ";

        // imprime las 4 palabras formateadas en hex de 8 dígitos
        for (size_t j = 0; j < 4 && (i + j) < cache_lines_.size(); ++j) {
            file << std::hex << std::setw(8) << std::setfill('0')
            << cache_lines_[i + j];
            if (j < 3 && (i + j + 1) < cache_lines_.size())
                file << " ";
        }

        file << "\n";
    }
    // vuelve a decimal por si hubiera futuros << file
    file << std::dec;
}
