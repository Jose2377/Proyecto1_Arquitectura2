// Memory.hpp

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <vector>
#include <mutex>
#include <string>

class Memory {
public:
    explicit Memory(const std::string& filename);

    void Initialize();
    void Write(uint32_t addr, uint32_t data);
    uint32_t Read(uint32_t addr);
    void Save();

private:
    std::string filename_;
    std::vector<uint32_t> mem_;
    std::mutex mem_mutex_;
};

#endif  // MEMORY_HPP_
