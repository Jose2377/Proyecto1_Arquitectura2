// ProcessorElement.cpp

#include "ProcessorElement.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

ProcessorElement::ProcessorElement(int id, Interconnect* interconnect)
: id_(id),
pe_path_("./PE" + std::to_string(id)),
instr_file_(pe_path_ + "/PE" + std::to_string(id) + "_instr.txt"),
cache_file_(pe_path_ + "/PE" + std::to_string(id) + "_cache.txt"),
interconnect_(interconnect),
cache_(cache_file_) {}

void ProcessorElement::Initialize() {
    if (!fs::exists(instr_file_)) {
        throw std::runtime_error("Missing " + instr_file_ + ". Create it first.");
    }
    if (!fs::exists(cache_file_)) {
        std::ofstream(cache_file_);
    }
    cache_.Initialize();
}

void ProcessorElement::LoadInstructions() {
    std::ifstream file(instr_file_);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;  // Evitar líneas vacías

        std::istringstream iss(line);
        Instruction instr;
        iss >> instr.message_type;

        if (instr.message_type == "WRITE_MEM" ||
            instr.message_type == "READ_MEM") {

            std::string src_hex, addr_hex, size_hex, qos_hex;
        iss >> src_hex >> addr_hex >> size_hex >> qos_hex;

        instr.src = static_cast<uint8_t>(std::stoul(src_hex, nullptr, 16));
        instr.addr = static_cast<uint32_t>(std::stoul(addr_hex, nullptr, 16));
        instr.size = static_cast<uint32_t>(std::stoul(size_hex, nullptr, 16));
        instr.qos = static_cast<uint8_t>(std::stoul(qos_hex, nullptr, 16));

            } else if (instr.message_type == "INV_ACK" || instr.message_type == "INV_COMPLETE") {
                std::string src_hex, qos_hex;
                iss >> src_hex >> qos_hex;

                instr.src = static_cast<uint8_t>(std::stoul(src_hex, nullptr, 16));
                instr.qos = static_cast<uint8_t>(std::stoul(qos_hex, nullptr, 16));
            } else if (instr.message_type == "BROADCAST_INVALIDATE") {
                std::string src_hex, addr_hex, size_hex; // Only 3 parameters
                iss >> src_hex >> addr_hex >> size_hex;

                instr.src = static_cast<uint8_t>(std::stoul(src_hex, nullptr, 16));
                instr.addr = static_cast<uint32_t>(std::stoul(addr_hex, nullptr, 16));
                instr.size = static_cast<uint32_t>(std::stoul(size_hex, nullptr, 16));
                instr.qos = 0; // Optional, if qos is not needed
            } else {
                std::cerr << "[Warning] Unknown instruction type: " << instr.message_type << "\n";
                continue;
            }

            instructions_.push_back(instr);
    }
}

void ProcessorElement::Run() {
    thread_ = std::thread([this]() {
        for (const auto& instr : instructions_) {
            std::cout << "[PE" << id_ << "] Sending: " << instr.message_type << std::endl;
            ProcessInstruction(instr);
            interconnect_->SendMessage(instr);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

void ProcessorElement::Join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

void ProcessorElement::ReceiveMessage(const Instruction& instr) {
    if (instr.message_type == "READ_RESP") {
        uint32_t cache_line = (instr.addr / 16) % 128;
        cache_.WriteLine(cache_line, instr.data);
        std::cout << "[PE" << id_ << "] Received READ_RESP and updated cache line " << cache_line << "\n";
    } else if (instr.message_type == "WRITE_RESP") {
        std::cout << "[PE" << id_ << "] Received WRITE_RESP status: "
        << (instr.status == 0x1 ? "OK" : "NOT_OK") << "\n";
    } else if (instr.message_type == "INV_COMPLETE") {
        std::cout << "[PE" << id_ << "] Received INV_COMPLETE\n";
    } else if (instr.message_type == "BROADCAST_INVALIDATE") {
        // Al recibir un Broadcast, mandar un INV_ACK automáticamente
        Instruction ack = {
            .message_type = "INV_ACK",
            .src = static_cast<uint8_t>(id_),
            .qos = instr.qos
        };
        interconnect_->SendMessage(ack);
        std::cout << "[PE" << id_ << "] Received BROADCAST_INVALIDATE, sending INV_ACK\n";
    }
}

void ProcessorElement::ProcessInstruction(const Instruction& instr) {
    uint32_t cache_line = (instr.addr / 16) % 128;
    if (instr.message_type == "WRITE_MEM") {
        uint32_t data = cache_.ReadLine(cache_line);
        std::cout << "[PE" << id_ << "] Preparing WRITE with data 0x"
        << std::hex << data << " from cache line " << cache_line << std::endl;
    } else if (instr.message_type == "READ_MEM") {
        std::cout << "[PE" << id_ << "] Preparing READ, will store into cache line " << cache_line << std::endl;
    }
}
