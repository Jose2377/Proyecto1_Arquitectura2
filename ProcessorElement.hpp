// ProcessorElement.hpp

#ifndef PROCESSORELEMENT_HPP_
#define PROCESSORELEMENT_HPP_

#include "Instruction.hpp"
#include "Interconnect.hpp"
#include "Cache.hpp"

#include <string>
#include <vector>
#include <thread>

class ProcessorElement {
public:
    ProcessorElement(int id, Interconnect* interconnect);

    void Initialize();
    void LoadInstructions();
    void Run();
    void Join();
    void ReceiveMessage(const Instruction& instr);

private:
    int id_;
    std::string pe_path_;
    std::string instr_file_;
    std::string cache_file_;
    std::vector<Instruction> instructions_;
    Interconnect* interconnect_;
    std::thread thread_;
    Cache cache_;

    void ProcessInstruction(const Instruction& instr);
};

#endif  // PROCESSORELEMENT_HPP_
