// ProcessorElement.cpp

#include "ProcessorElement.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;


ProcessorElement::ProcessorElement(int id, Interconnect* interconnect)
: id_(id), interconnect_(interconnect) {
    instr_file_ = "./PE" + std::to_string(id_) + "/PE" + std::to_string(id_) + "_instr.txt";
}

void ProcessorElement::Initialize() {
    if (!fs::exists(instr_file_)) {
        throw std::runtime_error("Missing " + instr_file_ + ". Create it first.");
    }
}

void ProcessorElement::LoadInstructions() {
    std::ifstream file(instr_file_);
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        Instruction instr;
        std::string src_hex, addr_hex, size_hex, qos_hex;
        iss >> instr.message_type >> src_hex >> addr_hex >> size_hex >> qos_hex;
        instr.src = static_cast<uint8_t>(std::stoul(src_hex, nullptr, 16));
        instr.addr = static_cast<uint32_t>(std::stoul(addr_hex, nullptr, 16));
        instr.size = static_cast<uint32_t>(std::stoul(size_hex, nullptr, 16));
        instr.qos = static_cast<uint8_t>(std::stoul(qos_hex, nullptr, 16));
        instructions_.push_back(instr);
    }
}

void ProcessorElement::Run() {
    thread_ = std::thread([this]() {
        for (const auto& instr : instructions_) {
            std::cout << "[PE" << id_ << "] Sending: " << instr.message_type << std::endl;
            interconnect_->SendMessage(instr);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Simulación ligera
        }
    });
}

void ProcessorElement::Join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}
