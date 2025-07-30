#include "../include/interfaz_terminal.hpp"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    try {
        // Procesar argumentos de línea de comandos o ejecutar interfaz interactiva
        InterfazTerminal::procesarArgumentos(argc, argv);
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error inesperado: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Error desconocido" << std::endl;
        return 1;
    }
    
    return 0;
} 