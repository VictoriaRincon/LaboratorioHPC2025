#ifndef ANALIZADOR_EXHAUSTIVO_HPP
#define ANALIZADOR_EXHAUSTIVO_HPP

#include "calculador_costos.hpp"
#include "escenario.hpp"
#include <fstream>
#include <chrono>
#include <bitset>
#include <limits>
// Estructura para almacenar resultados de una combinación
struct ResultadoCombinacion {
    uint32_t combinacion_id;           // ID de la combinación (0 a 2^24-1)
    std::bitset<24> patron_eolica;     // Patrón de energía eólica (0=0, 1=500)
    double costo_total;                // Costo total encontrado
    bool solucion_valida;              // Si se encontró solución válida
    int horas_criticas;                // Número de horas que requieren generación
    std::vector<EstadoMaquina> secuencia_optima; // Secuencia de estados óptima
    
    ResultadoCombinacion() : combinacion_id(0), costo_total(0.0), 
                            solucion_valida(false), horas_criticas(0) {}
};

// Estructura para estadísticas de progreso
struct EstadisticasProgreso {
    uint32_t combinaciones_procesadas;
    uint32_t combinaciones_totales;
    uint32_t soluciones_validas;
    double costo_minimo_global;
    double costo_maximo_global;
    double costo_promedio;
    std::chrono::steady_clock::time_point tiempo_inicio;
    std::chrono::steady_clock::time_point ultimo_reporte;
    
    EstadisticasProgreso() : combinaciones_procesadas(0), combinaciones_totales(16777216),
                            soluciones_validas(0), costo_minimo_global(std::numeric_limits<double>::infinity()),
                            costo_maximo_global(0.0), costo_promedio(0.0) {}
};

class AnalizadorExhaustivo {
private:
    std::vector<double> demanda_fija_;     // Demanda fija para las 24 horas
    std::ofstream archivo_resultados_;    // Archivo para guardar resultados
    std::ofstream archivo_log_;           // Archivo de log para progreso
    EstadisticasProgreso stats_;          // Estadísticas de progreso
    
    // Configuración
    uint32_t intervalo_reporte_;          // Cada cuántas combinaciones reportar progreso
    bool guardar_todas_soluciones_;       // Si guardar todas o solo las mejores
    double umbral_costo_interes_;         // Solo guardar soluciones bajo este costo
    
    // Métodos privados
    void configurarEscenario(Escenario& escenario, const std::bitset<24>& patron_eolica);
    void guardarResultado(const ResultadoCombinacion& resultado);
    void mostrarProgreso();
    void generarReporteProgreso();
    std::string estadoToString(EstadoMaquina estado) const;
    std::string tiempoTranscurrido() const;
    double tiempoEstimadoRestante() const;
    
public:
    // Constructor
    AnalizadorExhaustivo();
    
    // Configuración
    void configurarDemanda(const std::vector<double>& demanda);
    void configurarArchivos(const std::string& archivo_resultados, const std::string& archivo_log);
    void configurarReporte(uint32_t intervalo, bool guardar_todas = false, double umbral_costo = std::numeric_limits<double>::infinity());
    
    // Análisis principal
    void ejecutarAnalisisCompleto();
    void ejecutarAnalisisParcial(uint32_t desde, uint32_t hasta);
    
    // Utilidades
    void mostrarEstadisticasFinales();
    void generarResumenEjecutivo();
};

#endif // ANALIZADOR_EXHAUSTIVO_HPP
