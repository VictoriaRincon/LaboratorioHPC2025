#ifndef ANALIZADOR_EXHAUSTIVO_MPI_HPP
#define ANALIZADOR_EXHAUSTIVO_MPI_HPP

#include "calculador_costos.hpp"
#include "escenario.hpp"
#include <mpi.h>
#include <fstream>
#include <chrono>
#include <bitset>
#include <limits>
#include <sstream>

// Estructura para almacenar resultados de una combinación (compatible con MPI)
struct ResultadoCombinacionMPI {
    uint32_t combinacion_id;           
    uint32_t patron_eolica_bits;       // Patrón como entero para facilitar MPI
    double costo_total;                
    int solucion_valida;               // int en lugar de bool para MPI
    int horas_criticas;                
    
    ResultadoCombinacionMPI() : combinacion_id(0), patron_eolica_bits(0), 
                               costo_total(0.0), solucion_valida(0), horas_criticas(0) {}
};

// Estructura para estadísticas de progreso MPI
struct EstadisticasProgresoMPI {
    uint32_t combinaciones_procesadas;
    uint32_t combinaciones_totales;
    uint32_t soluciones_validas;
    double costo_minimo_global;
    double costo_maximo_global;
    double costo_promedio;
    std::chrono::steady_clock::time_point tiempo_inicio;
    std::chrono::steady_clock::time_point ultimo_reporte;
    
    EstadisticasProgresoMPI() : combinaciones_procesadas(0), combinaciones_totales(16777216),
                               soluciones_validas(0), costo_minimo_global(std::numeric_limits<double>::infinity()),
                               costo_maximo_global(0.0), costo_promedio(0.0) {}
};

class AnalizadorExhaustivoMPI {
private:
    std::vector<double> demanda_fija_;
    std::string archivo_resultados_;
    std::string archivo_log_;
    EstadisticasProgresoMPI stats_;
    
    // Variables MPI
    int rank_;
    int size_;
    
    // Configuración de reportes
    uint32_t intervalo_reporte_;
    bool guardar_todas_soluciones_;
    double umbral_costo_;
    
    // Buffer para comunicación MPI
    std::vector<ResultadoCombinacionMPI> buffer_resultados_;
    
    // Métodos auxiliares
    void configurarEscenario(Escenario& escenario, const std::bitset<24>& patron_eolica);
    void reportarProgreso();
    void guardarResultado(const ResultadoCombinacionMPI& resultado);
    void inicializarArchivos();
    void finalizarArchivos();
    
    // Métodos MPI específicos
    void calcularRangoTrabajo(uint32_t total_combinaciones, uint32_t& desde, uint32_t& hasta);
    void recolectarResultados();
    void enviarResultados();
    void fusionarEstadisticas();

public:
    // Constructor
    AnalizadorExhaustivoMPI();
    
    // Destructor
    ~AnalizadorExhaustivoMPI();
    
    // Configuración
    void configurarDemanda(const std::vector<double>& demanda);
    void configurarArchivos(const std::string& archivo_resultados, const std::string& archivo_log);
    void configurarReporte(uint32_t intervalo, bool guardar_todas = false, double umbral = 1000.0);
    
    // Ejecución principal MPI
    void ejecutarAnalisisCompletoMPI();
    void ejecutarAnalisisParcialMPI(uint32_t desde, uint32_t hasta);
    
    // Métodos de utilidad
    void mostrarEstadisticas() const;
    void mostrarConfiguracion() const;
    
    // Métodos MPI
    int getRank() const { return rank_; }
    int getSize() const { return size_; }
    bool esMaster() const { return rank_ == 0; }
};

// Funciones auxiliares MPI
void inicializarMPI(int argc, char* argv[]);
void finalizarMPI();

#endif // ANALIZADOR_EXHAUSTIVO_MPI_HPP 