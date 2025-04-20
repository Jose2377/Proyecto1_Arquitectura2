#include <iostream>
#include <sstream>
#include <cstdlib>
#include <bitset>
#include <fstream>
#include <string>

using namespace std;

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

// Clase para el Interconect
class Interconect{
    private: 
        //Memoria Compartida, 4096 memoria total (32 bits de ancho x 128 de largo)
        // Alineada en 4 byts
        bool memoria[32][128]; 
    public:
        Interconect(){
            for (int i = 0; i < 32; ++i){
                for (int j = 0; j < 128; ++j){
                    memoria[i][j] = 0;
                }
            }
        }
        // Escribe en memoria del interconect
        // Escribe de DEST(PE), en la direccion (DIR) escribe el valor (OBJ), con valor de prioridad (PRIO)
        string Write(string DEST, string DIR, string OBJ, string PRIO){
            // Obtiene valores
            int temp0 = DIR.length();
            DIR.erase(temp0-1,1);
            DIR.erase(0,2);

            temp0 = stoi(DIR);
            int L = temp0/128;
            int C = temp0%128;

            // Empieza a escribir en memoria cache
            string temp1 = OBJ;
            for (int i = 0; i < temp1.length(); ++i){
                if (temp1[i] == 49){
                    memoria[L][C] = 1;
                } else
                {
                    memoria[L][C] = 0;
                }
                C++;
                // Salta a la siguente linea en caso de overflow
                if (C == 128){
                    C = 0;
                    L++;
                }
            }

            temp1 = "WRITE_RESP ";
            temp1 = temp1 + DEST + ", ";
            temp1 = temp1 + "0x1, ";
            temp1 = temp1 + PRIO;
            return temp1;
        }
        // Imprime la memoria del Interconnect
        void Result(){
            ofstream MyFile("MemoryInterconnect.txt");
            for (int i = 0; i < 32; ++i){
                for (int j = 0; j < 128; ++j){
                    if (memoria[i][j] == 0){
                        MyFile << "0";
                    }
                    else {
                        MyFile << "1";
                    }
                    if (j != 0 && j%8 == 0){
                        MyFile << " ";
                    }
                }
                MyFile << "\n";
            }
            MyFile.close();
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
        }
        // Ejecucion de instrucciones
        void Ejecutar(){
            // Abre el archivo con las instrucciones correspondientes
            ifstream myfile ("PE"+to_string(name)+".txt");
            string mystring;
            string aux0;
            string aux1;
            string aux2;

            // Empieza a ejecutar las instrucciones
            if (myfile.is_open()) {     
                while ( myfile.good() ) {
                    myfile >> mystring;
                    KeepMessage(mystring + " ");
                    // Funcion de WRITE_CACHE (ADDRESS, VALUE)
                    if (mystring == "WRITE_CACHE"){
                        myfile >> mystring;
                        aux0 = mystring;
                        myfile >> mystring;
                        aux1 = mystring;
                        WriteCache(aux0,aux1);
                        KeepMessage(aux0 + " " + aux1 + "\n");
                    // Funcion de WRITE_MEM (ADDRESS in memory, NUMBER_OF_BYTES, ADDRESS in cache)
                    } else if (mystring == "WRITE_MEM"){
                        myfile >> mystring;
                        aux0 = mystring;
                        myfile >> mystring;
                        aux1 = mystring;
                        myfile >> mystring;
                        aux2 = mystring;
                        KeepMessage(aux0 + " " + aux1 + " " + aux2 + "\n");
                        aux2 = ReadCache(aux2,aux1);
                        aux1 = "0x" + to_string(name);
                        aux0 = INTERCONECT.Write(aux1,aux0,aux2,prioridad);
                        KeepMessage(aux0 + "\n");
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
                    if (j != 0 && j%8 == 0){
                        MyFile << " ";
                    }
                }
                MyFile << "\n";
            }
            MyFile.close();

            ofstream MyFile2("MessagesPE"+to_string(name)+".txt");
            MyFile2 << messages;
            MyFile2.close();
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
            OBJ.erase(0,2);
            temp0 = stoi(DIR);
            int L = temp0/128;
            int C = temp0%128;

            // Empieza a escribir en memoria cache
            string temp1 = hex_str_to_bin_str(OBJ);
            for (int i = 0; i < temp1.length(); ++i){
                if (temp1[i] == 49){
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
            return temp1;
        }
};

int main(){
    PE PE0(0,"0x00"); //NOMBRE_INSTANCIA(NUMERO_PE,PRIORIDAD)
    PE0.Ejecutar();
    INTERCONECT.Result();
    return 0;
    }
