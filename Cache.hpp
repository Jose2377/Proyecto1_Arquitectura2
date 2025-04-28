// Cache.hpp

#ifndef CACHE_HPP_
#define CACHE_HPP_

#include <vector>
#include <string>
#include <mutex>

class Cache {
public:
    explicit Cache(const std::string& filename);

    void Initialize();
    void WriteLine(uint32_t cache_line, uint32_t data);
    uint32_t ReadLine(uint32_t cache_line);
    void InvalidateLine(uint32_t cache_line);
    void Save();

private:
    static constexpr uint32_t kNumLines = 128;
    std::vector<uint32_t> cache_lines_;
    std::string filename_;
    std::mutex cache_mutex_;
};

#endif  // CACHE_HPP_
