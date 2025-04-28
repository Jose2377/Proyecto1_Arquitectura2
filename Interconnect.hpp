// Interconnect.hpp

#ifndef INTERCONNECT_HPP_
#define INTERCONNECT_HPP_

#include "Instruction.hpp"
#include "Memory.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <fstream>

class Interconnect {
public:
    explicit Interconnect(Memory* memory);

    void SendMessage(const Instruction& instr);
    void ProcessMessages();
    void Stop();
    void RegisterPE(int pe_id, class ProcessorElement* pe);
    void PrintStatistics() const;

private:
    std::queue<Instruction> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_flag_;
    Memory* memory_;
    std::ofstream traffic_log_;

    std::unordered_map<int, ProcessorElement*> pe_registry_;

    // Para INV_COMPLETE
    bool waiting_for_invacks_ = false;
    int expected_invacks_ = 0;
    int received_invacks_ = 0;
    int inv_origin_ = -1;

    // Estadísticas
    int total_messages_ = 0;
};

#endif  // INTERCONNECT_HPP_
