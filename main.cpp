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
        std::cout << "Seleccione modo de calendarización:\n";
        std::cout << "0 - FIFO\n";
        std::cout << "1 - QoS\n";
        std::cout << "2 - Salir\n";
        int option = -1;
        std::cin >> option;

        if (option == 2) {
            std::cout << "Saliendo.\n";
            return 0;
        }

        SchedulingMode mode;
        if (option == 0) {
            mode = SchedulingMode::FIFO;
            std::cout << "Calendarización seleccionada: FIFO\n";
        } else if (option == 1) {
            mode = SchedulingMode::QoS;
            std::cout << "Calendarización seleccionada: QoS\n";
        } else {
            std::cerr << "Opción inválida.\n";
            return 1;
        }

        Memory memory("./MEM.txt");
        memory.Initialize();

        Interconnect interconnect(&memory, mode);

        std::vector<std::unique_ptr<ProcessorElement>> pes;
        for (int i = 0; i < 4; ++i) {
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
