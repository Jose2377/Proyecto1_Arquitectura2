import os
import shutil
import glob

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

if __name__ == "__main__":
    print("Iniciando organización de archivos...")
    organize_files()
    print("Organización de archivos completada.")