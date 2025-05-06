import os
import shutil
import glob
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import random
from collections import defaultdict
import re

def clean_directory(directory_path):
    """Limpia todos los archivos dentro de un directorio."""
    if os.path.exists(directory_path):
        # Elimina todos los archivos dentro de la carpeta
        for file in os.listdir(directory_path):
            file_path = os.path.join(directory_path, file)
            if os.path.isfile(file_path):
                os.remove(file_path)
        print(f"Carpeta {directory_path} limpiada.")
    else:
        # Crea la carpeta si no existe
        os.makedirs(directory_path)
        print(f"Carpeta {directory_path} creada.")

def organize_files():
    """Organiza los archivos según el patrón especificado."""
    # Definimos las carpetas de destino
    memory_folder = "PE_memory"
    messages_folder = "PE_messages"
    
    # Limpiamos o creamos las carpetas de destino
    clean_directory(memory_folder)
    clean_directory(messages_folder)
    
    # Movemos los archivos MemoryPE#.txt a PE_memory
    for file in glob.glob("MemoryPE*.txt"):
        shutil.move(file, os.path.join(memory_folder, file))
        print(f"Movido {file} a {memory_folder}")
    
    # Movemos los archivos MessagesPE#.txt a PE_messages
    for file in glob.glob("MessagesPE*.txt"):
        shutil.move(file, os.path.join(messages_folder, file))
        print(f"Movido {file} a {messages_folder}")
    
    # Eliminamos los archivos PE_temp_#.txt
    for file in glob.glob("PE_temp_*.txt"):
        os.remove(file)
        print(f"Eliminado {file}")

# 1. Función para analizar el ancho de banda del Interconnect
def analyze_interconnect_bandwidth(num_cycles=10):
    """
    Analiza el ancho de banda del Interconnect basado en los archivos PE#.txt.
    Las instrucciones contribuyen al ancho de banda de la siguiente manera:
    - WRITE_CACHE: +0
    - READ_MEM o WRITE_MEM: +2
    - BROADCAST_INVALIDATE: +9
    
    Args:
        num_cycles: Número de ciclos a analizar
    """
    # Inicializar contador de carga por ciclo
    bandwidth_per_cycle = [0] * num_cycles
    
    # Analizar cada PE (0-7)
    for pe_num in range(8):
        filename = f"PE{pe_num}.txt"
        
        try:
            with open(filename, 'r') as file:
                lines = file.readlines()
                
                # Procesar hasta num_cycles líneas (o menos si el archivo es más corto)
                for cycle, line in enumerate(lines[:num_cycles]):
                    # Extraer el tipo de instrucción (primera palabra antes del espacio)
                    instruction_match = re.match(r'^(\w+)', line.strip())
                    if instruction_match:
                        instruction_type = instruction_match.group(1)
                        
                        # Calcular la carga según el tipo de instrucción
                        if instruction_type == "WRITE_CACHE":
                            load = 0
                        elif instruction_type in ["READ_MEM", "WRITE_MEM"]:
                            load = 2
                        elif instruction_type == "BROADCAST_INVALIDATE":
                            load = 9
                        else:
                            load = 0  # Instrucción desconocida
                        
                        # Añadir la carga al ciclo correspondiente
                        bandwidth_per_cycle[cycle] += load
            
            print(f"Procesado {filename}: {len(lines[:num_cycles])} instrucciones")
                
        except FileNotFoundError:
            print(f"Advertencia: No se encontró el archivo {filename}")
    
    return bandwidth_per_cycle

# 2. Función para analizar el tráfico en cada esquema
def analyze_pe_traffic(num_cycles=10, num_pes=8):
    """
    Analiza el tráfico de instrucciones por PE basado en los archivos PE#.txt.
    Las instrucciones contribuyen al tráfico de la siguiente manera:
    - WRITE_CACHE: +1 para el PE que lo ejecuta
    - READ_MEM o WRITE_MEM: +2 para el PE que lo ejecuta
    - BROADCAST_INVALIDATE: +2 para el PE que lo emite, +1 para todos los demás PEs
    
    Args:
        num_cycles: Número de ciclos a analizar
        num_pes: Número total de PEs en el sistema
    
    Returns:
        traffic_per_pe_cycle: Matriz con el tráfico por PE y ciclo
    """
    # Inicializar matriz de tráfico [ciclo][pe]
    traffic_per_pe_cycle = np.zeros((num_cycles, num_pes), dtype=int)
    
    # Primero leer todas las instrucciones de todos los PEs
    all_instructions = []
    for pe_num in range(num_pes):
        pe_instructions = []
        filename = f"PE{pe_num}.txt"
        
        try:
            with open(filename, 'r') as file:
                lines = file.readlines()
                for line in lines[:num_cycles]:
                    pe_instructions.append(line.strip())
                
                # Si hay menos instrucciones que ciclos, rellena con ''
                if len(pe_instructions) < num_cycles:
                    pe_instructions.extend([''] * (num_cycles - len(pe_instructions)))
                    
            print(f"Procesado {filename}: {len(lines[:num_cycles])} instrucciones")
                
        except FileNotFoundError:
            print(f"Advertencia: No se encontró el archivo {filename}")
            pe_instructions = [''] * num_cycles
            
        all_instructions.append(pe_instructions)
    
    # Analizar ciclo por ciclo
    for cycle in range(num_cycles):
        # Primero buscar si hay algún BROADCAST_INVALIDATE en este ciclo
        broadcast_pes = []
        for pe_num in range(num_pes):
            instr = all_instructions[pe_num][cycle] if cycle < len(all_instructions[pe_num]) else ''
            if instr and instr.startswith("BROADCAST_INVALIDATE"):
                broadcast_pes.append(pe_num)
        
        # Procesar instrucciones para cada PE en este ciclo
        for pe_num in range(num_pes):
            instr = all_instructions[pe_num][cycle] if cycle < len(all_instructions[pe_num]) else ''
            
            if instr:
                # Extraer el tipo de instrucción
                instruction_match = re.match(r'^(\w+)', instr)
                if instruction_match:
                    instruction_type = instruction_match.group(1)
                    
                    # Calcular el tráfico según el tipo de instrucción
                    if instruction_type == "WRITE_CACHE":
                        traffic_per_pe_cycle[cycle][pe_num] += 1
                    elif instruction_type in ["READ_MEM", "WRITE_MEM"]:
                        traffic_per_pe_cycle[cycle][pe_num] += 2
                    elif instruction_type == "BROADCAST_INVALIDATE":
                        traffic_per_pe_cycle[cycle][pe_num] += 2
        
        # Añadir +1 a todos los PEs excepto los que emitieron el broadcast
        for broadcast_pe in broadcast_pes:
            for pe_num in range(num_pes):
                if pe_num != broadcast_pe:
                    traffic_per_pe_cycle[cycle][pe_num] += 1
    
    return traffic_per_pe_cycle

# 3. Función para analizar los tiempos de ejecución
def analyze_execution_times(file_path="InstructionTiming.csv", pe_messages_dir="PE_messages"):
    """
    Analiza los tiempos de ejecución de las instrucciones.
    Agrega tiempos aleatorios entre 0.5 y 1.5 para INV_ACK.
    """
    if not os.path.exists(file_path):
        print(f"Error: El archivo {file_path} no existe")
        return None
    
    try:
        # Cargar el CSV
        df = pd.read_csv(file_path)
        
        # Verificar si el archivo tiene cabecera correcta
        if "Instruction Type" not in df.columns[0]:
            # Si no tiene cabecera, asignamos nombres manualmente
            df.columns = ["Instruction Type", "PE ID", "Data Size", "Execution Time", "Cycle"]
        
        # Inicializar estructura de datos para tiempos de ejecución por PE y ciclo
        execution_times = defaultdict(lambda: defaultdict(float))
        max_cycle = 0
        
        # Llenamos la estructura con los datos del CSV
        for idx, row in df.iterrows():
            try:
                # Extraer ciclo
                cycle_str = str(row["Cycle"])
                # Manejar posible formato hexadecimal
                if cycle_str.startswith("0x"):
                    cycle = int(cycle_str, 16)
                else:
                    cycle = int(cycle_str)
                    
                max_cycle = max(max_cycle, cycle)
                
                # Extraer PE ID
                pe_id_str = str(row["PE ID"])
                if pe_id_str.startswith("PE"):
                    pe_id = int(pe_id_str[2:])
                elif pe_id_str.startswith("0x"):
                    pe_id = int(pe_id_str, 16)
                else:
                    pe_id = int(pe_id_str)
                    
                exec_time = float(row["Execution Time"])
                execution_times[pe_id][cycle] = exec_time
            except Exception as e:
                print(f"Error al procesar fila {idx} del CSV: {e}")
                continue
        
        # Revisar los archivos PE_messages para encontrar BROADCAST_INVALIDATE
        if os.path.exists(pe_messages_dir):
            message_files = glob.glob(os.path.join(pe_messages_dir, "MessagesPE*.txt"))
            pe_count = len(message_files)
            
            # Analizar cada archivo de PE para determinar ciclos e instrucciones
            pe_instructions_by_cycle = {}  # {pe_id: {cycle: [instrucciones]}}
            broadcast_cycles = {}  # {cycle: pe_id_que_envio}
            
            for file_path in message_files:
                try:
                    pe_id = int(os.path.basename(file_path).replace("MessagesPE", "").replace(".txt", ""))
                    if pe_id not in pe_instructions_by_cycle:
                        pe_instructions_by_cycle[pe_id] = {}
                        
                    current_cycle = 0
                    instructions_in_current_cycle = []
                    
                    with open(file_path, 'r') as file:
                        for line in file:
                            line = line.strip()
                            if not line:
                                continue
                                
                            parts = line.split()
                            if not parts:
                                continue
                                
                            instruction_type = parts[0]
                            
                            # Añadir instrucción al ciclo actual
                            if instruction_type not in instructions_in_current_cycle:
                                instructions_in_current_cycle.append(instruction_type)
                            
                            # Identificar BROADCAST_INVALIDATE
                            if instruction_type == "BROADCAST_INVALIDATE":
                                broadcast_cycles[current_cycle] = pe_id
                            
                            # Si encontramos un WRITE_RESP o READ_RESP, terminamos el ciclo actual
                            if instruction_type in ["WRITE_RESP", "READ_RESP"]:
                                pe_instructions_by_cycle[pe_id][current_cycle] = instructions_in_current_cycle
                                max_cycle = max(max_cycle, current_cycle)
                                current_cycle += 1
                                instructions_in_current_cycle = []
                            # Si es WRITE_CACHE, también terminamos el ciclo
                            elif instruction_type == "WRITE_CACHE" and instructions_in_current_cycle == ["WRITE_CACHE"]:
                                pe_instructions_by_cycle[pe_id][current_cycle] = instructions_in_current_cycle
                                max_cycle = max(max_cycle, current_cycle)
                                current_cycle += 1
                                instructions_in_current_cycle = []
                        
                        # Si quedaron instrucciones pendientes, las asignamos al último ciclo
                        if instructions_in_current_cycle:
                            pe_instructions_by_cycle[pe_id][current_cycle] = instructions_in_current_cycle
                            max_cycle = max(max_cycle, current_cycle)
                except Exception as e:
                    print(f"Error al procesar el archivo {file_path} para tiempos de ejecución: {e}")
                    continue
            
            # Agregar tiempos aleatorios para INV_ACK
            for cycle, sender_pe in broadcast_cycles.items():
                for pe_id in range(pe_count):
                    if pe_id != sender_pe:
                        # Generar tiempo aleatorio entre 0.5 y 1.5
                        random_time = random.uniform(0.5, 1.5)
                        # Añadir el tiempo aleatorio
                        execution_times[pe_id][cycle] += random_time
        
        # Preparar los datos para graficar
        cycles = list(range(0, min(max_cycle + 1, 11)))
        exec_time_data = {}
        
        for pe_id in range(len(execution_times)):
            times = [execution_times[pe_id].get(cycle, 0) for cycle in cycles]
            exec_time_data[f"PE{pe_id}"] = times
        
        return cycles, exec_time_data
        
    except Exception as e:
        print(f"Error al procesar el archivo {file_path}: {e}")
        return None
    
# Función para generar las gráficas
def generate_graphs(bandwidth_data, traffic_data, num_pes=8):
    # 1. Gráfica de ancho de banda del Interconnect
    cycles = list(range(1, len(bandwidth_data) + 1))
    
    plt.figure(figsize=(12, 6))
    plt.bar(cycles, bandwidth_data, color='blue', alpha=0.7)
    plt.axhline(y=np.mean(bandwidth_data), color='r', linestyle='--', label=f'Promedio: {np.mean(bandwidth_data):.2f}')
    
    plt.title('Ancho de Banda del Interconnect por Ciclo')
    plt.xlabel('Ciclo')
    plt.ylabel('Carga en el Interconnect')
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.xticks(cycles)
    plt.legend()
    
    # Guardar el gráfico
    plt.savefig('1anchoBanda_interconnect.png')
    plt.close()
    
    print("Gráfico guardado como '1anchoBanda_interconnect.png'")
    
    # 2. Gráfica de tráfico por esquema
    num_cycles = traffic_data.shape[0]
    cycles = list(range(1, num_cycles + 1))
    
    # Gráfico 1: Tráfico total por ciclo
    plt.figure(figsize=(15, 10))
    plt.subplot(2, 1, 1)
    
    total_traffic_per_cycle = traffic_data.sum(axis=1)
    plt.bar(cycles, total_traffic_per_cycle, color='blue', alpha=0.7)
    plt.axhline(y=np.mean(total_traffic_per_cycle), color='r', linestyle='--', 
                label=f'Promedio: {np.mean(total_traffic_per_cycle):.2f}')
    
    plt.title('Tráfico Total del Sistema por Ciclo')
    plt.xlabel('Ciclo')
    plt.ylabel('Tráfico Total')
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.xticks(cycles)
    plt.legend()
    
    # Gráfico: Tráfico por PE
    plt.figure(figsize=(14, 8))
    
    width = 0.8 / num_pes
    for pe in range(num_pes):
        plt.bar([c + pe * width for c in cycles], traffic_data[:, pe], 
                width=width, label=f'PE{pe}', alpha=0.7)
    
    plt.title('Tráfico por PE y Ciclo', fontsize=16)
    plt.xlabel('Ciclo', fontsize=14)
    plt.ylabel('Tráfico', fontsize=14)
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.xticks(np.array(cycles) + width * num_pes / 2 - width / 2, cycles)
    plt.legend(fontsize=12)
    
    plt.tight_layout()
    plt.savefig('2trafico_PE.png')
    plt.close()
    
    print("Gráfico guardado como '2trafico_PE.png'")
    
    
    # 3. Gráfica de tiempos de ejecución
    exec_time_data = analyze_execution_times()
    if exec_time_data:
        cycles, exec_times_by_pe = exec_time_data
        plt.figure(figsize=(12, 7))
        
        # Crear un gráfico de líneas para cada PE
        for pe_name, times in exec_times_by_pe.items():
            plt.plot(cycles, times, marker='o', label=pe_name)
        
        plt.title('Tiempos de Ejecución por PE y Ciclo')
        plt.xlabel('Ciclo')
        plt.ylabel('Tiempo de Ejecución')
        plt.xticks(cycles)
        plt.legend()
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.tight_layout()
        plt.savefig('3tiempo_ejecucion.png')
        plt.close()
        print("Gráfica de tiempos de ejecución generada: 3tiempo_ejecucion.png")


if __name__ == "__main__":
    print("Iniciando organización de archivos...")
    organize_files()
    print("Organización de archivos completada.")
    print("Generando gráficas de rendimiento...")
    bandwidth_data = analyze_interconnect_bandwidth(num_cycles=10)
    traffic_data = analyze_pe_traffic(num_cycles=10, num_pes=8)
    generate_graphs(bandwidth_data, traffic_data)
    print("Proceso completado.")