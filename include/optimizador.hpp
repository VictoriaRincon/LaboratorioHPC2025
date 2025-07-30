#ifndef OPTIMIZADOR_HPP
#define OPTIMIZADOR_HPP

#include "estados_maquina.hpp"
#include <vector>
#include <unordered_map>
#include <memory>

struct Solucion {
    std::vector<EstadoMaquina> secuencia_estados;
    double costo_total;
    bool es_factible;
    
    Solucion() : costo_total(0.0), es_factible(false) {}
    Solucion(double costo, bool factible) : costo_total(costo), es_factible(factible) {}
};

struct EstadoRecursion {
    int hora;
    EstadoMaquina estado_actual;
    
    bool operator==(const EstadoRecursion& other) const {
        return hora == other.hora && estado_actual == other.estado_actual;
    }
};

// Hash para usar EstadoRecursion como clave en unordered_map
struct HashEstadoRecursion {
    size_t operator()(const EstadoRecursion& estado) const {
        return std::hash<int>()(estado.hora) ^ 
               (std::hash<int>()(static_cast<int>(estado.estado_actual)) << 1);
    }
};

class OptimizadorMaquina {
public:
    OptimizadorMaquina(const std::vector<int>& disponibilidad_eolica);
    
    // Resolver el problema completo
    Solucion resolver();
    
    // Resolver desde una hora específica hacia atrás
    Solucion resolverDesdeHora(int hora_inicial);
    
    // Obtener estadísticas del problema
    void mostrarEstadisticas() const;
    
    // Limpiar caché de memoización
    void limpiarCache();

private:
    std::vector<int> energia_eolica;  // Vector binario de disponibilidad eólica
    std::unordered_map<EstadoRecursion, Solucion, HashEstadoRecursion> memo;
    
    // Función recursiva principal con memoización
    Solucion resolverRecursivo(int hora, EstadoMaquina estado_llegada);
    
    // Reconstruir la secuencia de estados desde la solución
    void reconstruirSolucion(Solucion& solucion, int hora_inicial);
    
    // Verificar si el problema es factible
    bool esProblemaFactible() const;
    
    // Obtener tipo de escenario (crítico, óptimo, mixto)
    std::string obtenerTipoEscenario() const;
};

#endif // OPTIMIZADOR_HPP 