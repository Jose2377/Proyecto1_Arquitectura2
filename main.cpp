// main.cpp

#include "Memory.hpp"
#include "Interconnect.hpp"
#include "ProcessorElement.hpp"

#include <vector>
#include <memory>
#include <thread>
#include <iostream>

int main() {
    try {
        Memory memory("./MEM.txt");
        memory.Initialize();

        Interconnect interconnect(&memory);

        std::vector<std::unique_ptr<ProcessorElement>> pes;
        for (int i = 0; i < 2; ++i) {
            auto pe = std::make_unique<ProcessorElement>(i, &interconnect);
            pe->Initialize();
            pe->LoadInstructions();
            interconnect.RegisterPE(i, pe.get());
            pes.push_back(std::move(pe));
        }

        std::thread interconnect_thread(&Interconnect::ProcessMessages, &interconnect);

        for (auto& pe : pes) {
            pe->Run();
        }

        for (auto& pe : pes) {
            pe->Join();
        }

        interconnect.Stop();
        interconnect_thread.join();
        interconnect.PrintStatistics();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
