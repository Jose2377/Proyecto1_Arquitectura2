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

using namespace std;

int total_instruction = 0;
mutex instruction_mtx;
mutex interconnect_mtx;

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
            string temp1 = "READ_MEM " + DEST + ", " + DIR + ", " + OBJ + ", " + PRIO + "\n";
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
            ifstream myfile("PE"+to_string(name)+".txt"); // <-- Única declaración
            string mystring;
            string aux0;
            string aux1;
            string aux2;

            for (int i = 0; i < instruction; ++i){
                getline(myfile,mystring);
            }

            // Empieza a ejecutar las instrucciones
            if (myfile.is_open()) {     
                if ( myfile.good() ) {
                    myfile >> mystring;
                    KeepMessage(mystring + " ");
                    // Funcion de WRITE_CACHE (ADDRESS, VALUE)
                    if (mystring == "WRITE_CACHE"){
                        // Obtiene valores necesarios de la instruccion
                        myfile >> mystring;
                        aux0 = mystring;
                        myfile >> mystring;
                        aux1 = mystring;

                        // Guarda la instruccion en los mensajes
                        KeepMessage(aux0 + " " + aux1 + "\n");

                        // Cambia de hexadecimal string a binario string
                        aux1.erase(0,2);
                        aux1 = hex_str_to_bin_str(aux1);

                        // Escribe en cache
                        WriteCache(aux0, aux1);

                    // Funcion de WRITE_MEM (ADDRESS in memory, NUMBER_OF_BYTES, ADDRESS in cache)
                    } else if (mystring == "WRITE_MEM"){
                        // Obtiene valores necesarios de la instruccion
                        myfile >> mystring;
                        aux0 = mystring;
                        myfile >> mystring;
                        aux1 = mystring;
                        myfile >> mystring;
                        aux2 = mystring;

                        // Guarda la instruccion en los mensajes
                        KeepMessage(aux0 + " " + aux1 + " " + aux2 + "\n");

                        // Prepara datos, al leer cache y obtener el SRC
                        aux2 = ReadCache(aux2, aux1);
                        aux1 = "0x" + to_string(name);

                        // Envia el mensaje de escritura al Interconnect
                        aux0 = INTERCONECT.Write(aux1, aux0, aux2, prioridad);

                        // Guarda la instruccion en los mensajes
                        KeepMessage(aux0);

                    // Funcion de READ_MEM (ADDRESS in memory and cache)
                    } else if (mystring == "READ_MEM"){
                        // Obtiene valores necesarios de la instruccion
                        myfile >> mystring;
                        aux2 = mystring;
                        myfile >> mystring;

                        // Guarda la instruccion en los mensajes
                        KeepMessage(aux2 + " " + mystring + "\n");

                        // Obtiene el SRC y envia el mensaje de lectura al Interconnect
                        aux1 = "0x" + to_string(name);
                        aux0 = INTERCONECT.Read(aux1, aux2, mystring, prioridad);

                        // Guarda la instruccion en los mensajes
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

                        // Escribe en cahe
                        WriteCache(aux2, aux0);
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

int main() {
    vector<thread> pe_threads;
    
    // Esperar a que terminen todos los hilos
    cout << "Listo" << endl;
    try {
        int i = 0;
        while (i < 10) {
            if (kbhit()){
                // Lanzar todos los PEs en hilos
                pe_threads.emplace_back(pe_thread, ref(PE0));
                pe_threads.emplace_back(pe_thread, ref(PE1));
                pe_threads.emplace_back(pe_thread, ref(PE2));
                pe_threads.emplace_back(pe_thread, ref(PE3));
                pe_threads.emplace_back(pe_thread, ref(PE4));
                pe_threads.emplace_back(pe_thread, ref(PE5));
                pe_threads.emplace_back(pe_thread, ref(PE6));
                pe_threads.emplace_back(pe_thread, ref(PE7));

                //for (auto& t : pe_threads) {
                //    t.join();
                //}

                cout << "Ejecutando" << endl; 
                cout << getch() << endl;
                i++;
                Sleep(1000);
            }
        }
    } catch(const std::system_error& e)
    {
        std::cout << "Caught system_error with code "
                    "[" << e.code() << "] meaning "
                    "[" << e.what() << "]\n";
    }

    // Generar resultados finales
    INTERCONECT.Result();
    
    // Escribir estadísticas adicionales
    cout << "Simulacion completada. Total de instrucciones ejecutadas: " << total_instruction << endl;
    cout << "Resultados guardados en MemoryInterconnect.txt y MessagesInterconnect.txt" << endl;
    cout << "Version: " << __cplusplus << endl;

    return 0;
}