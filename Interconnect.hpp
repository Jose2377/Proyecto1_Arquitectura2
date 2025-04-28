// Interconnect.hpp

#ifndef INTERCONNECT_HPP_
#define INTERCONNECT_HPP_

#include "Instruction.hpp"
#include "Memory.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include <fstream>

enum class SchedulingMode {
    FIFO,
    QoS
};

class Interconnect {
public:
    explicit Interconnect(Memory* memory, SchedulingMode mode);

    void SendMessage(const Instruction& instr);
    void ProcessMessages();
    void Stop();
    void RegisterPE(int pe_id, class ProcessorElement* pe);
    void PrintStatistics() const;

private:
    std::vector<Instruction> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_flag_;
    Memory* memory_;
    std::ofstream traffic_log_;
    SchedulingMode mode_;

    std::unordered_map<int, ProcessorElement*> pe_registry_;

    bool waiting_for_invacks_ = false;
    int expected_invacks_ = 0;
    int received_invacks_ = 0;
    int inv_origin_ = -1;

    int total_messages_ = 0;
};

#endif  // INTERCONNECT_HPP_
