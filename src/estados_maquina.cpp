#include "../include/estados_maquina.hpp"
#include <stdexcept>

// Inicialización de variables estáticas
std::unordered_map<EstadoMaquina, std::vector<EstadoMaquina>> GestorEstados::transiciones_desde;
std::unordered_map<EstadoMaquina, std::vector<EstadoMaquina>> GestorEstados::transiciones_hacia;
bool GestorEstados::transiciones_inicializadas = false;

double GestorEstados::obtenerCosto(EstadoMaquina estado) {
    switch (estado) {
        case EstadoMaquina::ON_CALIENTE:  return 5.0;
        case EstadoMaquina::ON_TIBIO:     return 2.5;
        case EstadoMaquina::ON_FRIO:      return 1.0;
        case EstadoMaquina::OFF_CALIENTE: return 0.0;
        case EstadoMaquina::OFF_TIBIO:    return 0.0;
        case EstadoMaquina::OFF_FRIO:     return 0.0;
        default:
            throw std::invalid_argument("Estado de máquina no válido");
    }
}

bool GestorEstados::puedeGenerarEnergia(EstadoMaquina estado) {
    return estado == EstadoMaquina::ON_CALIENTE;
}

void GestorEstados::inicializarTransiciones() {
    if (transiciones_inicializadas) return;
    
    // Definir transiciones válidas según las reglas del problema:
    // ON/CALIENTE  → OFF/CALIENTE | ON/CALIENTE
    // OFF/CALIENTE → ON/TIBIO     | OFF/TIBIO
    // ON/TIBIO     → ON/CALIENTE  | OFF/CALIENTE
    // OFF/TIBIO    → ON/FRIO      | OFF/FRIO
    // ON/FRIO      → ON/TIBIO     | OFF/TIBIO
    // OFF/FRIO     → ON/FRIO      | OFF/FRIO
    
    transiciones_desde[EstadoMaquina::ON_CALIENTE] = {
        EstadoMaquina::OFF_CALIENTE, EstadoMaquina::ON_CALIENTE
    };
    
    transiciones_desde[EstadoMaquina::OFF_CALIENTE] = {
        EstadoMaquina::ON_TIBIO, EstadoMaquina::OFF_TIBIO
    };
    
    transiciones_desde[EstadoMaquina::ON_TIBIO] = {
        EstadoMaquina::ON_CALIENTE, EstadoMaquina::OFF_CALIENTE
    };
    
    transiciones_desde[EstadoMaquina::OFF_TIBIO] = {
        EstadoMaquina::ON_FRIO, EstadoMaquina::OFF_FRIO
    };
    
    transiciones_desde[EstadoMaquina::ON_FRIO] = {
        EstadoMaquina::ON_TIBIO, EstadoMaquina::OFF_TIBIO
    };
    
    transiciones_desde[EstadoMaquina::OFF_FRIO] = {
        EstadoMaquina::ON_FRIO, EstadoMaquina::OFF_FRIO
    };
    
    // Construir el mapa inverso (estados que pueden llegar a cada estado)
    for (auto& [origen, destinos] : transiciones_desde) {
        for (auto destino : destinos) {
            transiciones_hacia[destino].push_back(origen);
        }
    }
    
    transiciones_inicializadas = true;
}

std::vector<EstadoMaquina> GestorEstados::obtenerEstadosOrigen(EstadoMaquina estado_destino) {
    inicializarTransiciones();
    auto it = transiciones_hacia.find(estado_destino);
    if (it != transiciones_hacia.end()) {
        return it->second;
    }
    return std::vector<EstadoMaquina>();
}

std::vector<EstadoMaquina> GestorEstados::obtenerEstadosDestino(EstadoMaquina estado_origen) {
    inicializarTransiciones();
    auto it = transiciones_desde.find(estado_origen);
    if (it != transiciones_desde.end()) {
        return it->second;
    }
    return std::vector<EstadoMaquina>();
}

std::string GestorEstados::estadoAString(EstadoMaquina estado) {
    switch (estado) {
        case EstadoMaquina::ON_CALIENTE:  return "ON/CALIENTE";
        case EstadoMaquina::ON_TIBIO:     return "ON/TIBIO";
        case EstadoMaquina::ON_FRIO:      return "ON/FRIO";
        case EstadoMaquina::OFF_CALIENTE: return "OFF/CALIENTE";
        case EstadoMaquina::OFF_TIBIO:    return "OFF/TIBIO";
        case EstadoMaquina::OFF_FRIO:     return "OFF/FRIO";
        default:
            throw std::invalid_argument("Estado de máquina no válido");
    }
}

EstadoMaquina GestorEstados::stringAEstado(const std::string& str) {
    if (str == "ON/CALIENTE")  return EstadoMaquina::ON_CALIENTE;
    if (str == "ON/TIBIO")     return EstadoMaquina::ON_TIBIO;
    if (str == "ON/FRIO")      return EstadoMaquina::ON_FRIO;
    if (str == "OFF/CALIENTE") return EstadoMaquina::OFF_CALIENTE;
    if (str == "OFF/TIBIO")    return EstadoMaquina::OFF_TIBIO;
    if (str == "OFF/FRIO")     return EstadoMaquina::OFF_FRIO;
    
    throw std::invalid_argument("Cadena de estado no válida: " + str);
}

std::vector<EstadoMaquina> GestorEstados::obtenerTodosLosEstados() {
    return {
        EstadoMaquina::ON_CALIENTE,
        EstadoMaquina::ON_TIBIO,
        EstadoMaquina::ON_FRIO,
        EstadoMaquina::OFF_CALIENTE,
        EstadoMaquina::OFF_TIBIO,
        EstadoMaquina::OFF_FRIO
    };
}

bool GestorEstados::esTransicionValida(EstadoMaquina origen, EstadoMaquina destino) {
    inicializarTransiciones();
    auto estados_destino = obtenerEstadosDestino(origen);
    for (auto estado : estados_destino) {
        if (estado == destino) {
            return true;
        }
    }
    return false;
} 