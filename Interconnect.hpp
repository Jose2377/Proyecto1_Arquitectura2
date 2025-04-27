// Interconnect.hpp

#ifndef INTERCONNECT_HPP_
#define INTERCONNECT_HPP_

#include "Instruction.hpp"
#include "Memory.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>

class Interconnect {
public:
    explicit Interconnect(Memory* memory);

    void SendMessage(const Instruction& instr);
    void ProcessMessages();
    void Stop();

    void PrintStatistics() const;

private:
    std::queue<Instruction> message_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_flag_;
    Memory* memory_;
    std::ofstream traffic_log_;

    // Estadísticas
    int total_messages_ = 0;
};

#endif  // INTERCONNECT_HPP_
