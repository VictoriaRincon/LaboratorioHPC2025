#pragma once

#include <vector>
#include <string>

class Escenario {
public:
    // Constructor
    Escenario();
    
    // Cargar datos desde archivo
    bool cargarDesdeArchivo(const std::string& nombreArchivo);
    
    // Cargar datos desde array
    void cargarDesdeArray(const std::vector<int>& datos);
    
    // Getters
    const std::vector<int>& getEnergiasEolicas() const;
    int getTamaño() const;
    bool esEolicaSuficiente(int hora) const;
    
    // Análisis del escenario
    bool esCritico() const;
    bool esOptimo() const;
    bool esMixto() const;
    
    // Información estadística
    int getHorasConEolicaSuficiente() const;
    int getHorasQueRequierenGeneracion() const;
    
    // Debug
    void mostrarEstadisticas() const;

private:
    std::vector<int> energiasEolicas;  // Array binario: 0 = insuficiente, 1 = suficiente
}; 