#ifndef ESTADOS_MAQUINA_HPP
#define ESTADOS_MAQUINA_HPP

#include <vector>
#include <string>
#include <unordered_map>

enum class EstadoMaquina {
    ON_CALIENTE = 0,  // Costo: 5.0, Genera energía
    ON_TIBIO = 1,     // Costo: 2.5, No genera energía
    ON_FRIO = 2,      // Costo: 1.0, No genera energía
    OFF_CALIENTE = 3, // Costo: 0.0, No genera energía
    OFF_TIBIO = 4,    // Costo: 0.0, No genera energía
    OFF_FRIO = 5      // Costo: 0.0, No genera energía
};

class GestorEstados {
public:
    static const int NUM_ESTADOS = 6;
    
    // Obtener costo de un estado
    static double obtenerCosto(EstadoMaquina estado);
    
    // Verificar si un estado puede generar energía
    static bool puedeGenerarEnergia(EstadoMaquina estado);
    
    // Obtener estados desde los cuales se puede llegar a un estado dado
    static std::vector<EstadoMaquina> obtenerEstadosOrigen(EstadoMaquina estado_destino);
    
    // Obtener estados a los cuales se puede ir desde un estado dado
    static std::vector<EstadoMaquina> obtenerEstadosDestino(EstadoMaquina estado_origen);
    
    // Convertir estado a string para visualización
    static std::string estadoAString(EstadoMaquina estado);
    
    // Convertir string a estado
    static EstadoMaquina stringAEstado(const std::string& str);
    
    // Obtener todos los estados posibles
    static std::vector<EstadoMaquina> obtenerTodosLosEstados();
    
    // Verificar si una transición es válida
    static bool esTransicionValida(EstadoMaquina origen, EstadoMaquina destino);

private:
    static void inicializarTransiciones();
    static std::unordered_map<EstadoMaquina, std::vector<EstadoMaquina>> transiciones_desde;
    static std::unordered_map<EstadoMaquina, std::vector<EstadoMaquina>> transiciones_hacia;
    static bool transiciones_inicializadas;
};

#endif // ESTADOS_MAQUINA_HPP 