//#define _WIN32_WINNT 0x0500 // Es necesaria esta definicion para esconder ventana de consola

#include <iostream>
#include <conio.h>
#include <sstream>
#include <cstdlib>
#include <bitset>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <windows.h>
#include <queue>
#include <map>
#include <algorithm>

using namespace std;

int total_instruction = 0;
mutex instruction_mtx;
mutex interconnect_mtx;

enum SchedulerType { FIFO, QOS };
SchedulerType scheduler = QOS; // Cambiar a FIFO si se desea ignorar prioridad


const char* hex_char_to_bin(char c)
{
    switch(toupper(c))
    {
        case '0': return "0000";
        case '1': return "0001";
        case '2': return "0010";
        case '3': return "0011";
        case '4': return "0100";
        case '5': return "0101";
        case '6': return "0110";
        case '7': return "0111";
        case '8': return "1000";
        case '9': return "1001";
        case 'A': return "1010";
        case 'B': return "1011";
        case 'C': return "1100";
        case 'D': return "1101";
        case 'E': return "1110";
        case 'F': return "1111";
        default: return "0000";
    }
}
// Convierte de hexa a binario
string hex_str_to_bin_str(const string& hex)
{
    string bin;
    for(unsigned i = 0; i != hex.length(); ++i)
       bin += hex_char_to_bin(hex[i]);
    return bin;
}

// Convierte de hexa a decimal
int hex_str_to_dec_int(string hexVal) 
{ 
    int len = hexVal.size(); 
    int base = 1; 
    int dec_val = 0; 
  

    for (int i = len - 1; i >= 0; i--) { 
        if (hexVal[i] >= '0' && hexVal[i] <= '9') { 
            dec_val += (int(hexVal[i]) - 48) * base; 
            base = base * 16; 
        } 
        else if (hexVal[i] >= 'A' && hexVal[i] <= 'F') { 
            dec_val += (int(hexVal[i]) - 55) * base; 
            base = base * 16; 
        } 
    } 
    return dec_val; 
} 

// Función para convertir un número binario a hexadecimal
string bin_str_to_hex_str(string binario) {
    // Convertir el binario a decimal
    bitset<32> bits(binario); // asumimos que el tamaño máximo es 32 bits
    unsigned long decimal = bits.to_ulong();

    // Convertir el decimal a hexadecimal
    char hexadecimal[100]; // suficientemente grande para almacenar el resultado
    sprintf(hexadecimal, "%lX", decimal);

    return string(hexadecimal);
}

// Clase para el Interconect
class Interconect{
    private: 
        string messages;
        // Memoria Compartida, 4096 memoria total
        // Alineada en 4 byts (32 bits)
        bool memoria[32][128]; 
    public:
        Interconect(){
            messages = "";
            for (int i = 0; i < 32; ++i){
                for (int j = 0; j < 128; ++j){
                    memoria[i][j] = 0;
                }
            }
        }
        // Escribe en memoria del interconect
        // Escribe de DEST(PE), en la direccion (DIR) escribe el valor (OBJ), con valor de prioridad (PRIO)
        string Write(string DEST, string DIR, string OBJ, string PRIO){
            lock_guard<mutex> lock(interconnect_mtx);

            // Guarda la instruccion en los mensajes
            string temp1 = "WRITE_MEM " + DEST + ", " + DIR + " 0x" + bin_str_to_hex_str(OBJ) + ", " + PRIO + "\n";
            KeepMessage(temp1);

            // Obtiene valores
            int temp0 = DIR.length();
            DIR.erase(temp0-1,1);
            DIR.erase(0,2);



            temp0 = hex_str_to_dec_int(DIR);
            int L = temp0/128;
            int C = temp0%128;

            // Empieza a escribir en memoria cache
            temp1 = OBJ;
            for (size_t i = 0; i < temp1.length(); ++i) {
                if (temp1[i] == 49){
                    memoria[C][L] = 1;
                } else
                {
                    memoria[C][L] = 0;
                }
                C++;
                // Salta a la siguente linea en caso de overflow
                if (C == 32){
                    C = 0;
                    L++;
                }
            }
            // Guarda la instruccion en los mensajes
            temp1 = "WRITE_RESP " + DEST + ", 0x1, "+ PRIO + "\n";
            KeepMessage(temp1);

            // Devuelve el mensaje de memoria
            return temp1;
        }

        string Read(string DEST, string DIR, string OBJ, string PRIO){
            lock_guard<mutex> lock(interconnect_mtx);

            // Guarda la instruccion en los mensajes
            string temp1 = "READ_MEM " + DEST + ", " + DIR + " " + OBJ + " " + PRIO + "\n";
            KeepMessage(temp1);

            // Obtiene valores
            DIR.erase(DIR.find(","));
            
            DIR.erase(0,2);
            OBJ.erase(0,2);

            int temp0 = stoi(DIR);
            int L = temp0/128;
            int C = temp0%128;

            // Empieza a leer en memoria compartida
            temp0 = hex_str_to_dec_int(OBJ);
            temp1 = "";
            for (size_t i = 0; i < temp1.length(); ++i) {
                if (memoria[C][L] == 0){
                    temp1.append("0");
                }
                else {
                    temp1.append("1");
                }
                C++;
                // Salta a la siguente linea en caso de overflow
                if (C == 32){
                   C = 0;
                   L++;
                }
            }

            // Guarda la instruccion en los mensajes
            temp1 = "READ_RESP " + DEST + ", 0x" + bin_str_to_hex_str(temp1) + ", " + PRIO + "\n";
            KeepMessage(temp1);

            // Devuelve el valor de memoria compartida
            return temp1;
        }

        // Guarda los mensajes
        void KeepMessage(string sended){
            messages = messages + sended;
        }

        // Imprime la memoria y mensajes del Interconnect
        void Result(){
            ofstream MyFile("MemoryInterconnect.txt");
            for (int i = 0; i < 128; ++i){
                for (int j = 0; j < 32; ++j){
                    if (memoria[j][i] == 0){
                        MyFile << "0";
                    }
                    else {
                        MyFile << "1";
                    }
                    if (j != 0 && (j+1)%8 == 0){
                        MyFile << " ";
                    }
                }
                MyFile << "\n";
            }
            MyFile.close();

            ofstream MyFile2("MessagesInterconnect.txt");
            MyFile2 << messages;
            MyFile2.close();
        }
        //Reiniciar memoria
        void Reset(){
            messages = "";
            for (int i = 0; i < 32; ++i){
                for (int j = 0; j < 128; ++j){
                    memoria[i][j] = 0;
                }
            }
        }
        // Método en Interconect para manejar BROADCAST_INVALIDATE
        void BroadcastInvalidate(int src_pe, int cache_line, string qos) {
            lock_guard<mutex> lock(interconnect_mtx);
            
            // Guarda el mensaje del BROADCAST_INVALIDATE
            string broadcast_message = "BROADCAST_INVALIDATE (PE" + to_string(src_pe) + "), CACHE_LINE: " + to_string(cache_line) + ", QoS: " + qos + "\n";
            KeepMessage(broadcast_message);  // Guardamos el mensaje original del BroadcastInvalidate
            cout << broadcast_message << endl;
        
            // Recorre todos los PEs y envía la instrucción INV_BACK, excepto el PE que originó el BROADCAST_INVALIDATE
            for (int pe_id = 0; pe_id < 8; ++pe_id) {
                if (pe_id != src_pe) {
                    string message = "INV_BACK (PE" + to_string(pe_id) + "), QoS: " + qos + "\n";
                    KeepMessage(message);  // Guardamos la respuesta INV_BACK
                    cout << message << endl;
                }
            }
        
            // Después de que todos los PEs envían un INV_BACK, el PE que originó el broadcast envía INV_COMPLETE
            string inv_complete_message = "INV_COMPLETE (PE" + to_string(src_pe) + "), QoS: 0x02" + "\n";
            KeepMessage(inv_complete_message);  // Guardamos la respuesta INV_COMPLETE
            cout << inv_complete_message << endl;
        }
        

};

Interconect INTERCONECT; // NOMBRE TEMPORAL

// Clase para los PEs
class PE{
    // Atributos privados
    private:
        int name;              // Nombre (0x00-0x07)
        string prioridad;      // Prioridad (0x00-0xFF)
        string messages;       // Registro de mensajes
        bool memory[128][128]; // Memoria privada, 128 bloques de 16 bytes(128 bits)
        int instruction;       // Instruccion actual
    // Atributos publicos
    public:
    // Constructor
        PE(int number, string value){
            // Inicializamos nombre, prioridad y memoria del PE
            name = number;
            prioridad = value;
            for (int i = 0; i < 128; ++i){
                for (int j = 0; j < 128; ++j){
                    memory[i][j] = 0;
                }
            }
            string messages = "";
            instruction = 0;
        }
        // Ejecucion de instrucciones
        void Ejecutar() {
            lock_guard<mutex> lock(instruction_mtx);
            ifstream myfile((scheduler == QOS) ? "PE_temp_" + to_string(name) + ".txt" : "PE" + to_string(name) + ".txt");
            string mystring;
            string aux0;
            string aux1;
            string aux2;
            string qos;

            if (scheduler == FIFO) {
                for (int i = 0; i < instruction; ++i) {
                    getline(myfile, mystring);
                }
            }

            // Empieza a ejecutar las instrucciones
            if (myfile.is_open()) {     
                if ( myfile.good() ) {
                    myfile >> mystring;
                    KeepMessage(mystring + " ");
                    // Imprimir la instrucción que va a ejecutar con su tipo
                    if (mystring == "WRITE_CACHE"){
                        // Obtiene valores necesarios de la instrucción
                        myfile >> mystring;
                        aux0 = mystring;
                        myfile >> mystring;
                        aux1 = mystring;

                        // Imprime la instrucción con el tipo
                        cout << "PE (" << name << "), WRITE_CACHE (" << aux0 << "), (" << aux1 << ")\n";

                        // Guarda la instrucción en los mensajes
                        KeepMessage(aux0 + " " + aux1 + "\n");

                        // Cambia de hexadecimal string a binario string
                        aux1.erase(0,2);
                        aux1 = hex_str_to_bin_str(aux1);

                        // Escribe en cache
                        WriteCache(aux0, aux1);

                    } else if (mystring == "WRITE_MEM"){
                        // Obtiene valores necesarios de la instrucción
                        myfile >> mystring;
                        aux0 = mystring;
                        myfile >> mystring;
                        aux1 = mystring;
                        myfile >> mystring;
                        aux2 = mystring;
                        myfile >> mystring;
                        qos = mystring;

                        // Imprime la instrucción con el tipo
                        cout << "PE (" << name << "), WRITE_MEM (" << aux0 << "), (" << aux1 << "), (" << aux2 << "), (" << qos << ")\n";

                        // Guarda la instrucción en los mensajes
                        KeepMessage(aux0 + " " + aux1 + " " + aux2 + "\n");

                        // Prepara datos, al leer cache y obtener el SRC
                        aux2 = ReadCache(aux2, aux1);
                        aux1 = "0x" + to_string(name);

                        // Envia el mensaje de escritura al Interconnect
                        aux0 = INTERCONECT.Write(aux1, aux0, aux2, prioridad);

                        // Guarda la instrucción en los mensajes
                        KeepMessage(aux0);

                    } else if (mystring == "READ_MEM"){
                        // Obtiene valores necesarios de la instrucción
                        myfile >> mystring;
                        aux2 = mystring;
                        myfile >> mystring;
                        aux1 = mystring;
                        myfile >> mystring;
                        qos = mystring;

                        // Imprime la instrucción con el tipo
                        cout << "PE (" << name << "), READ_MEM (" << aux2 << "), (" << aux1 << "), (" << qos << ")\n";

                        // Guarda la instrucción en los mensajes
                        KeepMessage(aux2 + " " + mystring + "\n");

                        // Obtiene el SRC y envia el mensaje de lectura al Interconnect
                        aux1 = "0x" + to_string(name);
                        aux0 = INTERCONECT.Read(aux1, aux2, mystring, prioridad);

                        // Guarda la instrucción en los mensajes
                        KeepMessage(aux0);

                        // Del mensaje, obtiene el valor a escribir
                        aux0.erase(0,aux0.find(",") + 4);
                        int temp = aux0.length() - aux0.find(",");
                        aux0.erase(aux0.find(","), temp);
                        aux0 = hex_str_to_bin_str(aux0);

                        // Cantidad de bytes
                        mystring.erase(0,2);
                        temp = hex_str_to_dec_int(mystring);

                        // Si el resultado de lectura es 0, se actualiza la cantidad de bits necesitados
                        if (aux0 == "0000"){   
                            aux0 = "";
                            for (int i = 0; i<8*temp; i++){
                                aux0.append("0");
                            }
                        }

                        // Escribe en cache
                        WriteCache(aux2, aux0);
                    } else if (mystring == "BROADCAST_INVALIDATE") {
                        // Obtiene los parámetros necesarios
                        string cache_line = "0x1";  // ejemplo de línea de cache
                        string qos = "0x01";  // QoS para INV_BACK
        
                        // Realiza la invalidación y espera las respuestas
                        INTERCONECT.BroadcastInvalidate(name, stoi(cache_line, nullptr, 16), qos);
                    }
                }         
            }
            
            // Al terminar de ejecutar las instrucciones, empieza a escribir memoria y mensajes
            ofstream MyFile("MemoryPE"+to_string(name)+".txt");
            for (int i = 0; i < 128; ++i){
                for (int j = 0; j < 128; ++j){
                    if (memory[i][j] == 0){
                        MyFile << "0";
                    }
                    else {
                        MyFile << "1";
                    }
                    if (j != 0 && (j+1)%8 == 0){
                        MyFile << " ";
                    }
                }
                MyFile << "\n";
            }
            MyFile.close();

            ofstream MyFile2("MessagesPE"+to_string(name)+".txt");
            MyFile2 << messages;
            MyFile2.close();

            instruction++;
        };




        // Guarda los mensajes
        void KeepMessage(string sended){
            messages = messages + sended;
        }

        // Funcion WRITE_CACHE, escribe en address(DIR) el valor (OBJ)
        void WriteCache(string DIR, string OBJ){
            // Obtiene valores
            int temp0 = DIR.length();
            DIR.erase(temp0-1,1);
            DIR.erase(0,2);
            temp0 = stoi(DIR);
            int L = temp0/128;
            int C = temp0%128;

            // Empieza a escribir en memoria cache
            string temp1 = OBJ;
            for (size_t  i = 0; i < temp1.length(); ++i){
                if (temp1[i] == 49){ // 49 = "1" en ascci
                    memory[L][C] = 1;
                } else
                {
                    memory[L][C] = 0;
                }
                C++;
                // Salta a la siguente linea en caso de overflow
                if (C == 128){
                    C = 0;
                    L++;
                }
            }
        }
        // Lee cache en address(DIR) la cantidad de bytes(OBJ)
        string ReadCache(string DIR, string OBJ){
            // Obtiene valores
            DIR.erase(0,2);
            int temp0 = OBJ.length();
            OBJ.erase(temp0-1,1);
            OBJ.erase(0,2);
            temp0 = stoi(DIR);
            int L = temp0/128;
            int C = temp0%128;

            // Empieza a leer en memoria cache
            temp0 = hex_str_to_dec_int(OBJ);
            string temp1 = "";
            for (int i = 0; i < temp0*8; ++i){
                if (memory[L][C] == 0){
                    temp1.append("0");
                }
                else {
                    temp1.append("1");
                }
                C++;
                // Salta a la siguente linea en caso de overflow
                if (C == 128){
                    C = 0;
                    L++;
                }
            }
            // Devuelve el valor de memoria cache
            return temp1;
        }
        //Reiniciar memoria
        void Reset(){
            for (int i = 0; i < 128; ++i){
                for (int j = 0; j < 128; ++j){
                    memory[i][j] = 0;
                }
            }
            messages = "";
        }

        int getPriorityValue() const {
            string hex = prioridad;
            hex.erase(0,2); // remove 0x
            return hex_str_to_dec_int(hex);
        }
        
};

// Creación de 8 PEs
PE PE0(0, "0x0");
PE PE1(1, "0x1");
PE PE2(2, "0x2");
PE PE3(3, "0x3");
PE PE4(4, "0x4");
PE PE5(5, "0x5");
PE PE6(6, "0x6");
PE PE7(7, "0x7");

void pe_thread(PE& pe) {
    pe.Ejecutar();
    {
        lock_guard<mutex> lock(instruction_mtx);
        total_instruction++;
    }
}


// Modificar la estructura de Instruction para incluir el número de línea
struct Instruction {
    int qos_value;
    int pe_id;
    string line;
    int line_num;

    Instruction(int qos, int id, const string& ln, int num) 
        : qos_value(qos), pe_id(id), line(ln), line_num(num) {}

    // Ordenamiento correcto: menor QoS = mayor prioridad
    bool operator<(const Instruction& other) const {
        // Verificar si es WRITE_CACHE (siempre máxima prioridad)
        bool this_is_write_cache = (line.find("WRITE_CACHE") != string::npos);
        bool other_is_write_cache = (other.line.find("WRITE_CACHE") != string::npos);
        
        if (this_is_write_cache && !other_is_write_cache)
            return true;  // this tiene mayor prioridad (debe aparecer antes)
        if (!this_is_write_cache && other_is_write_cache)
            return false; // other tiene mayor prioridad
            
        // Si ambos son WRITE_CACHE o ninguno lo es, ordenamos por QoS
        if (qos_value != other.qos_value) {
            return qos_value < other.qos_value; // Menor valor QoS = mayor prioridad
        }
        
        // En caso de mismo QoS, usar FIFO (orden por número de línea)
        return line_num < other.line_num;
    }
};


int main() {
    // Selección de algoritmo de calendarización
    cout << "Seleccione el tipo de calendarización:" << endl;
    cout << "1. FIFO (se ignora el QoS)" << endl;
    cout << "2. QoS (según prioridad asignada a cada PE)" << endl;
    cout << "Opción: ";
    int opcion;
    cin >> opcion;
    if (opcion == 2) {
        scheduler = QOS;
        cout << "Se utilizará calendarización basada en QoS." << endl;
    } else {
        scheduler = FIFO;
        cout << "Se utilizará calendarización FIFO." << endl;
    }

    // Vector de punteros a PEs y contador de líneas leídas por PE
    vector<PE*> pes = { &PE0, &PE1, &PE2, &PE3, &PE4, &PE5, &PE6, &PE7 };
    int instructionCount[8] = { 0 };

    cout << "Listo" << endl;
    int ciclos = 0, maxCiclos = 10;
    while (ciclos < maxCiclos) {
        if (!kbhit()) continue;               // Espera tecla para avanzar
        cout << "\n--- Ciclo " << ciclos << " ---\n";

        if (scheduler == FIFO) {
            // FIFO puro: un hilo por PE en orden 0…7
            vector<thread> threads;
            for (PE* pe : pes) {
                threads.emplace_back(pe_thread, ref(*pe));
            }
            for (auto &t : threads) t.join();
            total_instruction += pes.size();
        }
        else {
            // QoS: recolectar UNA instrucción pendiente de cada PE
            vector<Instruction> batch;
            for (int id = 0; id < 8; ++id) {
                ifstream f("PE" + to_string(id) + ".txt");
                string linea;
                // Saltar las instrucciones ya consumidas
                for (int s = 0; s < instructionCount[id] && getline(f, linea); ++s);
                // Leer siguiente instrucción
                if (!getline(f, linea)) {
                    cout << "PE" << id << " sin más instrucciones (consumidas: "
                         << instructionCount[id] << ")\n";
                    continue;
                }
                

                // Extraer opcode y QoS
                istringstream iss(linea);
                string opcode; iss >> opcode;
                int qosVal = 255;
                if (opcode == "WRITE_CACHE") {
                    qosVal = 0;
                } else if (opcode == "WRITE_MEM") {
                    string a1,a2,a3,qos; iss >> a1 >> a2 >> a3 >> qos;
                    if (qos.rfind("0x",0) == 0)
                        qosVal = hex_str_to_dec_int(qos.substr(2));
                } else if (opcode == "READ_MEM") {
                    string a1,a2,qos; iss >> a1 >> a2 >> qos;
                    if (qos.rfind("0x",0) == 0)
                        qosVal = hex_str_to_dec_int(qos.substr(2));
                }

                batch.emplace_back(qosVal, id, linea, instructionCount[id]);
            }

            // Ordenar por WRITE_CACHE primero, luego por QoS y FIFO
            sort(batch.begin(), batch.end());

            // Ejecutar las instrucciones en orden QoS
            vector<thread> threads;
            for (auto &inst : batch) {
                // Volcar esa línea en un archivo temporal
                ofstream tmp("PE_temp_" + to_string(inst.pe_id) + ".txt", ios::trunc);
                tmp << inst.line << "\n";
                tmp.close();

                // Lanzar hilo para ejecutar UNA instrucción de ese PE
                threads.emplace_back(pe_thread, ref(*pes[inst.pe_id]));

                // Actualizar contadores
                instructionCount[inst.pe_id]++;  
                total_instruction++;
            }
            for (auto &t : threads) t.join();
        }

        cout << "Presione cualquier tecla para continuar al siguiente ciclo..." << endl;
        getch();
        ciclos++;
    }

    // Resultados finales
    INTERCONECT.Result();
    cout << "\nSimulación completada. Total de instrucciones ejecutadas: "
         << total_instruction << endl;
    cout << "Resultados guardados en MemoryInterconnect.txt y MessagesInterconnect.txt\n";
    cout << "Versión de C++: " << __cplusplus << endl;
    cout << "Presione cualquier tecla para salir..." << endl;
    getch();
    return 0;
}
