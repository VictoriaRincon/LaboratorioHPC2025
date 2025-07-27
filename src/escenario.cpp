#include "escenario.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

Escenario::Escenario() {
    demanda_.resize(24, 0.0);
    energia_otras_fuentes_.resize(24, 0.0);
}

bool Escenario::cargarDatos(const std::string& archivo_parametros) {
    std::ifstream archivo(archivo_parametros);
    if (!archivo.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << archivo_parametros << std::endl;
        return false;
    }
    
    std::string linea;
    int hora = 0;
    
    // Leer demanda (primera línea)
    if (std::getline(archivo, linea)) {
        std::istringstream iss(linea);
        double valor;
        hora = 0;
        while (iss >> valor && hora < 24) {
            demanda_[hora] = valor;
            hora++;
        }
    }
    
    // Leer energía de otras fuentes (segunda línea)
    if (std::getline(archivo, linea)) {
        std::istringstream iss(linea);
        double valor;
        hora = 0;
        while (iss >> valor && hora < 24) {
            energia_otras_fuentes_[hora] = valor;
            hora++;
        }
    }
    
    archivo.close();
    return true;
}

double Escenario::getDemanda(int hora) const {
    if (hora >= 0 && hora < 24) {
        return demanda_[hora];
    }
    return 0.0;
}

double Escenario::getEnergiaOtrasFuentes(int hora) const {
    if (hora >= 0 && hora < 24) {
        return energia_otras_fuentes_[hora];
    }
    return 0.0;
}

bool Escenario::demandaCubiertaConEO(int hora) const {
    if (hora >= 0 && hora < 24) {
        return energia_otras_fuentes_[hora] >= demanda_[hora];
    }
    return false;
}

void Escenario::mostrarDatos() const {
    std::cout << "\n=== DATOS DEL ESCENARIO ===" << std::endl;
    std::cout << "Hora\tDemanda\tEO\tCubierta" << std::endl;
    std::cout << "----\t-------\t--\t--------" << std::endl;
    
    for (int hora = 0; hora < 24; hora++) {
        std::cout << hora << "\t" 
                  << demanda_[hora] << "\t" 
                  << energia_otras_fuentes_[hora] << "\t"
                  << (demandaCubiertaConEO(hora) ? "Sí" : "No") << std::endl;
    }
    std::cout << std::endl;
}

void Escenario::configurarDirecto(const std::vector<double>& demanda, const std::vector<double>& energia_otras_fuentes) {
    if (demanda.size() != 24 || energia_otras_fuentes.size() != 24) {
        throw std::invalid_argument("Los vectores deben tener exactamente 24 elementos");
    }
    
    demanda_ = demanda;
    energia_otras_fuentes_ = energia_otras_fuentes;
}

void Escenario::setDemanda(int hora, double valor) {
    if (hora >= 0 && hora < 24) {
        demanda_[hora] = valor;
    }
}

void Escenario::setEnergiaOtrasFuentes(int hora, double valor) {
    if (hora >= 0 && hora < 24) {
        energia_otras_fuentes_[hora] = valor;
    }
}
