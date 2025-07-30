#ifndef BENCHMARK_SISTEMA_HPP
#define BENCHMARK_SISTEMA_HPP

#include "optimizador.hpp"
#include <vector>
#include <chrono>
#include <map>

struct EstadisticasCombinacion {
    std::vector<int> combinacion;
    double costo_solucion;
    double tiempo_ejecucion_ms;
    bool solucion_factible;
    size_t estados_cache_utilizados;
};

struct ResultadoBenchmark {
    int largo_problema;
    long long total_combinaciones;
    long long combinaciones_factibles;
    
    // Tiempos
    double tiempo_total_ms;
    double tiempo_promedio_ms;
    double tiempo_minimo_ms;
    double tiempo_maximo_ms;
    double desviacion_estandar_ms;
    
    // Rendimiento
    double combinaciones_por_segundo;
    double tiempo_por_bit_ms;
    
    // Costos
    double costo_promedio;
    double costo_minimo;
    double costo_maximo;
    
    // Uso de memoria/cache
    size_t promedio_estados_cache;
    size_t maximo_estados_cache;
    
    // Distribución por tipo de escenario
    int escenarios_criticos;    // Todos 0s
    int escenarios_optimos;     // Todos 1s
    int escenarios_mixtos;      // Combinación
    
    std::vector<EstadisticasCombinacion> detalle_combinaciones;
};

class BenchmarkSistema {
public:
    // Constructor
    BenchmarkSistema();
    
    // Ejecutar benchmark completo para un largo dado
    ResultadoBenchmark ejecutarBenchmark(int largo_problema, bool mostrar_progreso = true);
    
    // Ejecutar benchmark con límite de tiempo
    ResultadoBenchmark ejecutarBenchmarkConLimite(int largo_problema, double limite_segundos, bool mostrar_progreso = true);
    
    // Generar todas las combinaciones binarias de longitud N
    std::vector<std::vector<int>> generarTodasLasCombinaciones(int largo);
    
    // Analizar una combinación específica
    EstadisticasCombinacion analizarCombinacion(const std::vector<int>& combinacion);
    
    // Mostrar resultados del benchmark
    void mostrarResultados(const ResultadoBenchmark& resultado);
    
    // Mostrar análisis de escalabilidad
    void mostrarAnalisisEscalabilidad(const std::vector<ResultadoBenchmark>& resultados);
    
    // Exportar resultados a archivo CSV
    void exportarCSV(const ResultadoBenchmark& resultado, const std::string& archivo);
    
    // Configuración del benchmark
    void configurarVerbosidad(bool verbose) { verboso = verbose; }
    void configurarDetalleCompleto(bool detalle) { incluir_detalle_completo = detalle; }

private:
    bool verboso;
    bool incluir_detalle_completo;
    
    // Funciones auxiliares para estadísticas
    double calcularPromedio(const std::vector<double>& valores);
    double calcularDesviacionEstandar(const std::vector<double>& valores, double promedio);
    double calcularMinimo(const std::vector<double>& valores);
    double calcularMaximo(const std::vector<double>& valores);
    
    // Clasificar tipo de escenario
    std::string clasificarEscenario(const std::vector<int>& combinacion);
    
    // Mostrar barra de progreso
    void mostrarProgreso(int actual, int total, double tiempo_transcurrido);
    
    // Formatear tiempo para visualización
    std::string formatearTiempo(double ms);
    
    // Formatear velocidad
    std::string formatearVelocidad(double ops_por_segundo);
};

// Funciones de utilidad para análisis comparativo
namespace AnalisisBenchmark {
    // Comparar resultados de diferentes largos
    void compararRendimiento(const std::vector<ResultadoBenchmark>& resultados);
    
    // Predecir rendimiento para largos mayores
    void predecirEscalabilidad(const std::vector<ResultadoBenchmark>& resultados, int largo_objetivo);
    
    // Análisis de complejidad empírica
    void analizarComplejidad(const std::vector<ResultadoBenchmark>& resultados);
    
    // Generar reporte completo
    void generarReporteCompleto(const std::vector<ResultadoBenchmark>& resultados, const std::string& archivo);
}

#endif // BENCHMARK_SISTEMA_HPP 