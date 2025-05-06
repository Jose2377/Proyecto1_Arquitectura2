import os
import shutil
import glob
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import random
from collections import defaultdict

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
def analyze_interconnect_bandwidth(file_path="MessagesInterconnect.txt", pe_messages_dir="PE_messages"):
    """
    Analiza el ancho de banda del Interconnect basado en los mensajes.
    Utiliza los archivos PE_messages para determinar qué instrucciones afectan al Interconnect.
    Cada WRITE_MEM o READ_MEM tiene una respuesta del Interconnect.
    Cada Broadcast tiene 7 respuestas INV_ACK y 1 INV_COMPLETE.
    """
    if not os.path.exists(file_path):
        print(f"Error: El archivo {file_path} no existe")
        return None
        
    if not os.path.exists(pe_messages_dir):
        print(f"Error: El directorio {pe_messages_dir} no existe")
        return None
    
    # Obtener la lista de archivos PE_messages
    message_files = glob.glob(os.path.join(pe_messages_dir, "MessagesPE*.txt"))
    
    if not message_files:
        print(f"Error: No se encontraron archivos MessagesPE*.txt en {pe_messages_dir}")
        return None
    
    # Inicializar contador de instrucciones por ciclo
    bandwidth_by_cycle = defaultdict(int)
    max_cycle = 0
    
    # Analizar los archivos PE_messages para determinar qué instrucciones afectan al Interconnect y sus ciclos
    pe_instructions_by_cycle = {}  # {pe_id: {cycle: [instrucciones]}}
    
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
            print(f"Error al procesar el archivo {file_path} para ancho de banda: {e}")
            continue
    
    # Ahora calculamos el ancho de banda basado en las instrucciones que afectan al Interconnect
    for pe_id, cycles in pe_instructions_by_cycle.items():
        for cycle, instructions in cycles.items():
            for instruction in instructions:
                # Si es WRITE_CACHE, no afecta al Interconnect
                if instruction == "WRITE_CACHE":
                    continue
                
                # Las demás instrucciones sí afectan al Interconnect
                bandwidth_by_cycle[cycle] += 1
                
                # WRITE_MEM o READ_MEM tienen una respuesta adicional
                if instruction in ["WRITE_MEM", "READ_MEM"]:
                    bandwidth_by_cycle[cycle] += 1
                
                # BROADCAST_INVALIDATE tiene 7 INV_ACK y 1 INV_COMPLETE
                if instruction == "BROADCAST_INVALIDATE":
                    bandwidth_by_cycle[cycle] += 8
    
    # Asegurarse de que hay datos para todos los ciclos hasta el máximo
    cycles = list(range(0, min(max_cycle + 1, 11)))
    bandwidth = [bandwidth_by_cycle.get(cycle, 0) for cycle in cycles]
    
    return cycles, bandwidth

# 2. Función para analizar el tráfico en cada esquema
def analyze_traffic_by_scheme(pe_messages_dir="PE_messages"):
    """
    Analiza el tráfico en cada esquema usando los archivos PE_messages.
    Cada PE ejecuta solo 1 instrucción por ciclo.
    Si es WRITE_MEM o READ_MEM, reciben una respuesta (2 instrucciones en total).
    Si alguien hace un BROADCAST, se suma 1 instrucción a todos los PEs.
    """
    if not os.path.exists(pe_messages_dir):
        print(f"Error: El directorio {pe_messages_dir} no existe")
        return None
    
    # Obtener la lista de archivos PE_messages
    message_files = glob.glob(os.path.join(pe_messages_dir, "MessagesPE*.txt"))
    
    if not message_files:
        print(f"Error: No se encontraron archivos MessagesPE*.txt en {pe_messages_dir}")
        return None
    
    # Inicializar contador de mensajes por PE y por ciclo
    traffic_by_pe_cycle = defaultdict(lambda: defaultdict(int))
    pe_count = len(message_files)
    max_cycle = 0
    
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
            print(f"Error al procesar el archivo {file_path} para tráfico: {e}")
            continue
    
    # Calcular el tráfico basado en las instrucciones y reglas
    for pe_id, cycles in pe_instructions_by_cycle.items():
        for cycle, instructions in cycles.items():
            # Siempre hay 1 instrucción base por ciclo
            traffic_by_pe_cycle[pe_id][cycle] += 1
            
            # Si hay READ_MEM o WRITE_MEM, añadir 1 más por la respuesta
            if any(instr in ["READ_MEM", "WRITE_MEM"] for instr in instructions):
                traffic_by_pe_cycle[pe_id][cycle] += 1
    
    # Agregar 1 instrucción a todos los PEs en ciclos con BROADCAST
    for cycle, sender_pe in broadcast_cycles.items():
        for pe_id in range(pe_count):
            traffic_by_pe_cycle[pe_id][cycle] += 1
    
    # Preparar los datos para graficar
    cycles = list(range(0, min(max_cycle + 1, 11)))
    traffic_data = {}
    
    for pe_id in range(pe_count):
        traffic = [traffic_by_pe_cycle[pe_id].get(cycle, 0) for cycle in cycles]
        traffic_data[f"PE{pe_id}"] = traffic
    
    return cycles, traffic_data

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
def generate_graphs():
    # 1. Gráfica de ancho de banda del Interconnect
    bandwidth_data = analyze_interconnect_bandwidth()
    if bandwidth_data:
        cycles, bandwidth = bandwidth_data
        plt.figure(figsize=(10, 6))
        plt.bar(cycles, bandwidth, color='blue')
        plt.title('Ancho de Banda del Interconnect por Ciclo')
        plt.xlabel('Ciclo')
        plt.ylabel('Número de Instrucciones')
        plt.xticks(cycles)
        plt.grid(axis='y', linestyle='--', alpha=0.7)
        plt.savefig('bandwidth_graph.png')
        plt.close()
        print("Gráfica de ancho de banda generada: bandwidth_graph.png")
    
    # 2. Gráfica de tráfico por esquema
    traffic_data = analyze_traffic_by_scheme()
    if traffic_data:
        cycles, traffic_by_pe = traffic_data
        plt.figure(figsize=(12, 7))
        
        bar_width = 0.1
        index = np.arange(len(cycles))
        
        for i, (pe_name, traffic) in enumerate(traffic_by_pe.items()):
            plt.bar(index + i*bar_width, traffic, bar_width, label=pe_name)
        
        plt.title('Tráfico por PE y Ciclo')
        plt.xlabel('Ciclo')
        plt.ylabel('Número de Instrucciones')
        plt.xticks(index + bar_width * (len(traffic_by_pe) / 2 - 0.5), cycles)
        plt.legend()
        plt.grid(axis='y', linestyle='--', alpha=0.7)
        plt.tight_layout()
        plt.savefig('traffic_graph.png')
        plt.close()
        print("Gráfica de tráfico generada: traffic_graph.png")
    
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
        plt.savefig('execution_time_graph.png')
        plt.close()
        print("Gráfica de tiempos de ejecución generada: execution_time_graph.png")


if __name__ == "__main__":
    print("Iniciando organización de archivos...")
    organize_files()
    print("Organización de archivos completada.")
    print("Generando gráficas de rendimiento...")
    generate_graphs()
    print("Proceso completado.")