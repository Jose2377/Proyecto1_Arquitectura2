// main.cpp

#include "Memory.hpp"
#include "Interconnect.hpp"
#include "ProcessorElement.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <thread>

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
            pes.push_back(std::move(pe));
        }

        // Thread para el Interconnect
        std::thread interconnect_thread(&Interconnect::ProcessMessages, &interconnect);

        // Ejecutar los PEs
        for (auto& pe : pes) {
            pe->Run();
        }

        // Esperar a los PEs
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
