# Makefile para compilar y ejecutar el simulador de Interconnect

CC = g++
CFLAGS = -std=c++20 -Wall -I. -pthread
SRCS = main.cpp ProcessorElement.cpp Memory.cpp Interconnect.cpp Cache.cpp
OBJS = $(SRCS:.cpp=.o)
EXEC = simulator
PE_DIRS = PE0 PE1  # Directorios necesarios para los PEs

.PHONY: all build create_pes run clean

all: build

build: $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Crear directorios de PEs (necesarios para la ejecución)
create_pes:
	mkdir -p $(PE_DIRS)
	@echo "\nCree los archivos PE*/PE*_instr.txt manualmente antes de ejecutar"

run: build
	@if [ ! -d "PE0" ] || [ ! -d "PE1" ]; then \
        echo "Error: Falta directorios PE0/ o PE1/"; \
        echo "Ejecute 'make create_pes' para crearlos"; \
        exit 1; \
    fi
	./$(EXEC)

clean:
	rm -rf $(EXEC) $(OBJS) MEM.txt MEM_interconnect.txt
