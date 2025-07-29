#ifndef CACHE_PREFIJOS_MPI_HPP
#define CACHE_PREFIJOS_MPI_HPP

#include <unordered_map>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <mpi.h>
#include "escenario.hpp"
#include "calculador_costos.hpp"

// Estructura para almacenar información de cache
struct EntradaCache {
    std::vector<EstadoMaquina> estados_parciales;
    double costo_acumulado;
    int frecuencia_uso;
    double tiempo_computo_ms;
    std::chrono::steady_clock::time_point ultimo_uso;
    
    EntradaCache() : costo_acumulado(0.0), frecuencia_uso(0), tiempo_computo_ms(0.0) {}
    
    EntradaCache(const std::vector<EstadoMaquina>& estados, double costo, double tiempo) 
        : estados_parciales(estados), costo_acumulado(costo), frecuencia_uso(1), 
          tiempo_computo_ms(tiempo), ultimo_uso(std::chrono::steady_clock::now()) {}
};

// Estructura para comunicación MPI de prefijos
struct PrefijoMPI {
    char patron[25];  // Máximo 24 chars + terminador
    double costo;
    int frecuencia;
    double tiempo_ms;
    int rank_origen;
    
    PrefijoMPI() : costo(0.0), frecuencia(0), tiempo_ms(0.0), rank_origen(-1) {
        memset(patron, 0, sizeof(patron));
    }
};

class CachePrefijos {
private:
    std::unordered_map<std::string, EntradaCache> cache_local;
    int rank;
    int size;
    
    // Parámetros de configuración
    double umbral_tiempo_cache_ms = 10.0;  // Solo cachear si el cómputo tomó > 10ms
    int min_frecuencia_intercambio = 3;    // Solo intercambiar prefijos usados >= 3 veces
    size_t max_cache_size = 10000;         // Límite de entradas en cache
    
    // Estadísticas
    int hits_cache = 0;
    int misses_cache = 0;
    int prefijos_enviados = 0;
    int prefijos_recibidos = 0;
    
    // Logging
    std::ofstream log_cache;
    bool logging_enabled;

public:
    CachePrefijos(int _rank, int _size) : rank(_rank), size(_size), logging_enabled(false) {}
    
    // Inicializar logging
    void habilitarLogging(bool habilitar = true);
    
    // Cerrar archivo de log
    void cerrarLog();
    
    // Escribir entrada de log
    void escribirLog(const std::string& tipo, const std::string& patron, 
                    double costo = 0.0, double tiempo = 0.0, int frecuencia = 0);
    
    // Buscar en cache local
    bool buscarPrefijo(const std::string& patron, EntradaCache& resultado);
    
    // Agregar al cache si cumple criterios
    void agregarPrefijo(const std::string& patron, const std::vector<EstadoMaquina>& estados, 
                       double costo, double tiempo_computo);
    
    // Limpiar cache cuando se llena
    void limpiarCacheAntiguo();
    
    // Intercambio de prefijos entre procesos
    void intercambiarPrefijosValiosos();
    
    // Recibir prefijos del proceso 0 (distribuidos)
    void recibirPrefijosDistribuidos();
    
    // Estadísticas del cache
    void mostrarEstadisticas() const;
    
    // Getters para estadísticas
    int getHits() const { return hits_cache; }
    int getMisses() const { return misses_cache; }
    size_t getTamaño() const { return cache_local.size(); }
    
    // Configuración
    void configurar(double umbral_tiempo, int min_freq, size_t max_size) {
        umbral_tiempo_cache_ms = umbral_tiempo;
        min_frecuencia_intercambio = min_freq;
        max_cache_size = max_size;
    }
};

// Función auxiliar para convertir estados a string
std::string estadosAString(const std::vector<EstadoMaquina>& estados);

// Función auxiliar para convertir string a estados
std::vector<EstadoMaquina> stringAEstados(const std::string& patron);

#endif // CACHE_PREFIJOS_MPI_HPP
