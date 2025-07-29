#include "../include/escenario.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

Escenario::Escenario() {
    // Constructor vacío
}

bool Escenario::cargarDesdeArchivo(const std::string& nombreArchivo) {
    std::ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << nombreArchivo << std::endl;
        return false;
    }
    
    std::string linea;
    if (!std::getline(archivo, linea)) {
        std::cerr << "Error: Archivo vacío o no se pudo leer" << std::endl;
        return false;
    }
    
    // Saltar líneas que empiecen con #
    while (linea.empty() || linea[0] == '#') {
        if (!std::getline(archivo, linea)) {
            std::cerr << "Error: No se encontraron datos válidos" << std::endl;
            return false;
        }
    }
    
    energiasEolicas.clear();
    std::istringstream iss(linea);
    int valor;
    
    while (iss >> valor) {
        if (valor != 0 && valor != 1) {
            std::cerr << "Error: Los valores deben ser 0 o 1. Encontrado: " << valor << std::endl;
            return false;
        }
        energiasEolicas.push_back(valor);
    }
    
    if (energiasEolicas.empty()) {
        std::cerr << "Error: No se encontraron datos válidos en el archivo" << std::endl;
        return false;
    }
    
    archivo.close();
    return true;
}

void Escenario::cargarDesdeArray(const std::vector<int>& datos) {
    energiasEolicas = datos;
}

const std::vector<int>& Escenario::getEnergiasEolicas() const {
    return energiasEolicas;
}

int Escenario::getTamaño() const {
    return static_cast<int>(energiasEolicas.size());
}

bool Escenario::esEolicaSuficiente(int hora) const {
    if (hora < 0 || hora >= getTamaño()) {
        return false;
    }
    return energiasEolicas[hora] == 1;
}

bool Escenario::esCritico() const {
    return std::all_of(energiasEolicas.begin(), energiasEolicas.end(), 
                       [](int valor) { return valor == 0; });
}

bool Escenario::esOptimo() const {
    return std::all_of(energiasEolicas.begin(), energiasEolicas.end(), 
                       [](int valor) { return valor == 1; });
}

bool Escenario::esMixto() const {
    return !esCritico() && !esOptimo();
}

int Escenario::getHorasConEolicaSuficiente() const {
    return static_cast<int>(std::count(energiasEolicas.begin(), energiasEolicas.end(), 1));
}

int Escenario::getHorasQueRequierenGeneracion() const {
    return static_cast<int>(std::count(energiasEolicas.begin(), energiasEolicas.end(), 0));
}

void Escenario::mostrarEstadisticas() const {
    std::cout << "=== ANÁLISIS DETALLADO DEL ESCENARIO ===" << std::endl;
    std::cout << "Horas con energía eólica suficiente: " << getHorasConEolicaSuficiente() 
              << "/" << getTamaño() << std::endl;
    std::cout << "Horas que requieren generación: " << getHorasQueRequierenGeneracion() 
              << "/" << getTamaño() << std::endl;
    
    if (esCritico()) {
        std::cout << "⚠️  ESCENARIO CRÍTICO: Todas las horas requieren generación" << std::endl;
    } else if (esOptimo()) {
        std::cout << "✅ ESCENARIO ÓPTIMO: Toda la demanda se cubre con energía eólica" << std::endl;
    } else {
        std::cout << "⚖️  ESCENARIO MIXTO: Optimización necesaria" << std::endl;
    }
    std::cout << std::endl;
} 