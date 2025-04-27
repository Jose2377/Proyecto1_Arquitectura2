// Interconnect.cpp

#include "Interconnect.hpp"

#include <iostream>
#include <thread>
#include <chrono>

Interconnect::Interconnect(Memory* memory) : memory_(memory), stop_flag_(false) {
    traffic_log_.open("./MEM_interconnect.txt", std::ios::out | std::ios::trunc);
}

void Interconnect::SendMessage(const Instruction& instr) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        message_queue_.push(instr);
        ++total_messages_;
    }
    cv_.notify_one();
}

void Interconnect::ProcessMessages() {
    while (!stop_flag_ || !message_queue_.empty()) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        cv_.wait(lock, [this]() { return !message_queue_.empty() || stop_flag_; });

        if (!message_queue_.empty()) {
            Instruction instr = message_queue_.front();
            message_queue_.pop();
            lock.unlock();

            // Simula procesamiento
            std::cout << "[Interconnect] Processing: " << instr.message_type << " from PE "
            << std::hex << static_cast<int>(instr.src) << std::endl;

            if (instr.message_type == "WRITE_MEM") {
                memory_->Write(instr.addr, 0xDEADBEEF);  // Solo para probar
            } else if (instr.message_type == "READ_MEM") {
                memory_->Read(instr.addr);
            }

            traffic_log_ << "SRC: 0x" << std::hex << static_cast<int>(instr.src)
            << " TYPE: " << instr.message_type
            << " ADDR: 0x" << instr.addr
            << " SIZE: 0x" << instr.size
            << " QoS: 0x" << static_cast<int>(instr.qos) << "\n";
        }
    }
}

void Interconnect::Stop() {
    stop_flag_ = true;
    cv_.notify_all();
}

void Interconnect::PrintStatistics() const {
    std::cout << "\n[Interconnect] Total messages processed: " << total_messages_ << "\n";
}
