#ifndef ESCENARIO_HPP
#define ESCENARIO_HPP

#include <vector>
#include <string>

class Escenario {
private:
    std::vector<double> demanda_;  // Demanda para cada hora (0-23)
    std::vector<double> energia_otras_fuentes_;  // Energía de otras fuentes para cada hora (0-23)
    
public:
    // Constructor
    Escenario();
    
    // Cargar datos desde archivo
    bool cargarDatos(const std::string& archivo_parametros);
    
    // Getters
    double getDemanda(int hora) const;
    double getEnergiaOtrasFuentes(int hora) const;
    
    // Verificar si la demanda se cubre con energía de otras fuentes
    bool demandaCubiertaConEO(int hora) const;
    
    // Mostrar información
    void mostrarDatos() const;
    
    // Métodos para configuración directa (sin archivo)
    void configurarDirecto(const std::vector<double>& demanda, const std::vector<double>& energia_otras_fuentes);
    void setDemanda(int hora, double valor);
    void setEnergiaOtrasFuentes(int hora, double valor);
};

#endif // ESCENARIO_HPP
