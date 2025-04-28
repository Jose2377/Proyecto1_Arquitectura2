// Interconnect.cpp

#include "Interconnect.hpp"
#include "ProcessorElement.hpp"

#include <iostream>
#include <algorithm>

Interconnect::Interconnect(Memory* memory, SchedulingMode mode)
: memory_(memory), stop_flag_(false), mode_(mode) {
    traffic_log_.open("./MEM_interconnect.txt", std::ios::out | std::ios::trunc);
}

void Interconnect::RegisterPE(int pe_id, ProcessorElement* pe) {
    pe_registry_[pe_id] = pe;
}

void Interconnect::SendMessage(const Instruction& instr) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        message_queue_.push_back(instr);
        ++total_messages_;
    }
    cv_.notify_one();
}

void Interconnect::ProcessMessages() {
    while (!stop_flag_ || !message_queue_.empty()) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        cv_.wait(lock, [this]() { return !message_queue_.empty() || stop_flag_; });

        if (!message_queue_.empty()) {
            // ✨ Aquí decidimos FIFO o QoS
            Instruction instr;
            if (mode_ == SchedulingMode::FIFO) {
                instr = message_queue_.front();
                message_queue_.erase(message_queue_.begin());
            } else { // QoS
                auto it = std::max_element(message_queue_.begin(), message_queue_.end(),
                                           [](const Instruction& a, const Instruction& b) {
                                               return a.qos < b.qos; // mayor primero
                                           });
                instr = *it;
                message_queue_.erase(it);
            }
            lock.unlock();

            traffic_log_ << "Processing: " << instr.message_type
            << " SRC: 0x" << std::hex << static_cast<int>(instr.src)
            << " ADDR: 0x" << instr.addr << "\n";

            std::cout << "[Interconnect] Processing: " << instr.message_type << " from PE "
            << static_cast<int>(instr.src) << std::endl;

            if (instr.message_type == "WRITE_MEM") {
                memory_->Write(instr.addr, 0xDEADBEEF);
                Instruction resp = {
                    .message_type = "WRITE_RESP",
                    .src = 0,
                    .dest = instr.src,
                    .qos = instr.qos,
                    .status = 0x1
                };
                pe_registry_[instr.src]->ReceiveMessage(resp);
            } else if (instr.message_type == "READ_MEM") {
                uint32_t data = memory_->Read(instr.addr);
                Instruction resp = {
                    .message_type = "READ_RESP",
                    .src = 0,
                    .dest = instr.src,
                    .addr = instr.addr,
                    .qos = instr.qos,
                    .data = data
                };
                pe_registry_[instr.src]->ReceiveMessage(resp);
            } else if (instr.message_type == "BROADCAST_INVALIDATE") {
                waiting_for_invacks_ = true;
                expected_invacks_ = pe_registry_.size();
                received_invacks_ = 0;
                inv_origin_ = instr.src;
                for (auto& [id, pe] : pe_registry_) {
                    if (id != instr.src) {
                        Instruction broadcast = {
                            .message_type = "BROADCAST_INVALIDATE",
                            .src = instr.src,
                            .dest = static_cast<uint8_t>(id),
                            .qos = instr.qos
                        };
                        pe->ReceiveMessage(broadcast);
                    }
                }
            } else if (instr.message_type == "INV_ACK") {
                ++received_invacks_;
                if (waiting_for_invacks_ && received_invacks_ >= expected_invacks_ - 1) {
                    Instruction complete = {
                        .message_type = "INV_COMPLETE",
                        .src = 0,
                        .dest = static_cast<uint8_t>(inv_origin_),
                        .qos = instr.qos
                    };
                    pe_registry_[inv_origin_]->ReceiveMessage(complete);
                    waiting_for_invacks_ = false;
                }
            }
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
