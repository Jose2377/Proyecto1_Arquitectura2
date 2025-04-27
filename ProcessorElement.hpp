// ProcessorElement.hpp

#ifndef PROCESSORELEMENT_HPP_
#define PROCESSORELEMENT_HPP_

#include "Instruction.hpp"
#include "Interconnect.hpp"

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

private:
    int id_;
    std::string instr_file_;
    std::vector<Instruction> instructions_;
    Interconnect* interconnect_;
    std::thread thread_;
};

#endif  // PROCESSORELEMENT_HPP_
