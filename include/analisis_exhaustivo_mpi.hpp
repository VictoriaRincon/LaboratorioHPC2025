#pragma once

#include <vector>
#include <string>
#include <map>
#include <mpi.h>
#include "calculador_costos.hpp"
#include "escenario.hpp"

// Estructura para resultados de una combinación (simplificada para benchmarking)
struct ResultadoCombinacion {
    std::vector<int> combinacion;
    std::vector<std::pair<int, std::string>> encendidos; // posición, estado (F/T/C)
    double costoTotal;
    bool valida;
};

// Estructura para caché distribuido
struct CacheEntrada {
    std::vector<int> patron;
    std::vector<Estado> solucion;
    double costo;
};

// Estructura para métricas de rendimiento por proceso
struct MetricasProceso {
    int rank;
    int combinacionesAsignadas;
    int combinacionesProcesadas;
    double tiempoAnalisis;
    double tiempoInicializacion;
    double tiempoFinalizacion;
    size_t memoriaUsada;
    int cacheHits;
    int cacheMisses;
};

// Estructura para métricas globales del benchmark
struct MetricasGlobales {
    int totalBits;
    long long totalCombinaciones;
    int numeroNucleos;
    int procesosUtilizados;
    double tiempoTotal;
    double tiempoDistribucion;
    double tiempoRecoleccion;
    double combinacionesPorSegundo;
    double speedup;
    double eficienciaParalela;
    double balanceCarga;
    size_t totalBytesTransferidos;
    int totalMensajesMPI;
    std::vector<MetricasProceso> metricasPorProceso;
};

// Estructura para configuración del benchmark
struct ConfiguracionBenchmark {
    int bits;
    int nucleos;
    int procesos;
    bool modoVerboso;
    bool guardarResultados;
    std::string archivoMetricas;
};

// Estructura para estadísticas de comunicación MPI
struct EstadisticasMPI {
    int mensajesEnviados = 0;
    int mensajesRecibidos = 0;
    size_t bytesEnviados = 0;
    size_t bytesRecibidos = 0;
    double tiempoDistribucion = 0.0;
    double tiempoRecoleccion = 0.0;
};

// Estructura para estadísticas de caché
struct EstadisticasCache {
    int hits = 0;
    int misses = 0;
    int patronesGuardados = 0;
    double tiempoTotal = 0.0;
};

class AnalisisExhaustivoMPI {
public:
    // Constructor
    AnalisisExhaustivoMPI(int rank, int size);
    
    // Método principal para benchmark de rendimiento
    void ejecutarBenchmark(const ConfiguracionBenchmark& config);
    
    // Método original para análisis exhaustivo (mantener compatibilidad)
    void ejecutarAnalisisExhaustivo(int longitud, const std::string& archivoSalida);
    
    // Generación de combinaciones
    void generarTodasLasCombinaciones(int longitud, std::vector<std::vector<int>>& combinaciones);
    
    // Análisis de una combinación específica (simplificado para benchmark)
    ResultadoCombinacion analizarCombinacion(const std::vector<int>& combinacion);
    bool procesarCombinacionRapido(const std::vector<int>& combinacion); // Solo validar sin detalles
    
    // Manejo de caché distribuido
    void sincronizarCache();
    void enviarCacheATodos();
    void recibirCacheDeOtros();
    
    // Utilidades para benchmark
    void inicializarBenchmark(const ConfiguracionBenchmark& config);
    void recolectarMetricas();
    void calcularMetricasGlobales();
    void escribirMetricasCSV(const std::string& archivo);
    void escribirMetricasRobustas(const ConfiguracionBenchmark& config, double tiempoTotal);
    void escribirMetricasIndividuales(const std::string& archivoBase, int rank, double tiempoTotal);
    void escribirArchivoConsolidado(const ConfiguracionBenchmark& config, double tiempoTotal);
    void escribirMetricasProgresivas(const ConfiguracionBenchmark& config, const std::string& estado, double progreso);
    void mostrarResumenBenchmark();
    
    // Caché distribuido MPI
    void sincronizarCacheDistribuido();
    void compartirSufijosEncontrados(const std::vector<int>& combinacion, const std::vector<Estado>& solucion);
    bool buscarSufijoEnCacheDistribuido(const std::vector<int>& sufijo, std::vector<Estado>& solucionEncontrada);
    void enviarPatronAOtrosProcesos(const std::vector<int>& patron, const std::vector<Estado>& solucion);
    void recibirPatronesDeOtrosProcesos();
    
    // Utilidades originales (mantener compatibilidad)
    std::string formatearEstadoEncendido(Estado estadoAnterior, Estado estadoActual, int posicion);
    std::string analizarTransicion(Estado estadoAnterior, Estado estadoActual, size_t posicion);
    void escribirCSV(const std::vector<ResultadoCombinacion>& resultados, const std::string& archivo);
    void escribirCSVIncremental(const std::vector<ResultadoCombinacion>& resultados, const std::string& archivo);
    void inicializarArchivoCSV(const std::string& archivo);
    
    // Estadísticas y progreso
    void mostrarProgreso(int combinacionActual, int totalCombinaciones);
    void mostrarProgresoSimplificado(int combinacionActual, int totalCombinaciones, double velocidad);
    void mostrarEstadisticasFinales(double tiempoTotal, int totalCombinaciones);
    void registrarMensajeMPI(bool enviado, size_t bytes);
    
    // Distribución de trabajo MPI
    std::vector<std::vector<int>> obtenerCombinacionesParaProceso(
        const std::vector<std::vector<int>>& todasCombinaciones);

private:
    int rank_;
    int size_;
    CalculadorCostos calculador_;
    std::map<std::vector<int>, CacheEntrada> cacheDistribuido_;
    
    // Métricas y configuración para benchmark
    ConfiguracionBenchmark config_;
    MetricasProceso metricasLocales_;
    MetricasGlobales metricasGlobales_;
    
    // Estadísticas
    EstadisticasMPI estadisticasMPI_;
    EstadisticasCache estadisticasCache_;
    
    // Constantes MPI
    static const int TAG_CACHE_SYNC = 100;
    static const int TAG_CACHE_DATA = 101;
    static const int TAG_CACHE_PATTERN = 102;
    static const int TAG_CACHE_SOLUTION = 103;
    static const int TAG_CACHE_COUNT = 104;
    static const int TAG_FINALIZE = 200;
    static const int TAG_METRICS = 300;
}; 