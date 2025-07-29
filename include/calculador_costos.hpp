#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <limits>
#include "escenario.hpp"

// Enumeración de estados de la máquina
enum class Estado {
    ON_CALIENTE = 0,
    ON_TIBIO = 1,
    ON_FRIO = 2,
    OFF_CALIENTE = 3,
    OFF_TIBIO = 4,
    OFF_FRIO = 5
};

// Estructura para información de subsecuencias (mantenida por compatibilidad)
struct SubsecuenciaInfo {
    std::vector<int> patron;      // El patrón de 0s y 1s
    int posicionInicio;           // Posición en el array original
    int posicionFin;              // Posición final en el array original
    int longitud;                 // Longitud de la subsecuencia
    
    SubsecuenciaInfo(const std::vector<int>& p, int inicio, int fin) 
        : patron(p), posicionInicio(inicio), posicionFin(fin), longitud(fin - inicio + 1) {}
};

// Estructura para el resultado de la optimización
struct ResultadoOptimizacion {
    std::vector<Estado> estados;  // Secuencia de estados óptima
    double costoTotal;           // Costo total de la solución
    bool solucionValida;         // Indica si se encontró una solución válida
    int horasOptimizadas;        // Número de horas optimizadas
};

// Estructura para memoización
struct EstadoMemo {
    double costo;
    bool valido;
    Estado estadoAnterior;
    
    EstadoMemo() : costo(std::numeric_limits<double>::infinity()), valido(false), estadoAnterior(Estado::OFF_FRIO) {}
    EstadoMemo(double c, bool v, Estado e) : costo(c), valido(v), estadoAnterior(e) {}
};

class CalculadorCostos {
public:
    // Constructor
    CalculadorCostos();
    
    // Método principal para resolver el problema
    ResultadoOptimizacion resolver(const Escenario& escenario);
    ResultadoOptimizacion resolver(const Escenario& escenario, int horaInicial, int horaFinal = 0);
    
    // Método optimizado que usa caché integrado al backtracking
    ResultadoOptimizacion resolverConPatrones(const Escenario& escenario);
    
    // Utilidades públicas
    static std::string estadoAString(Estado estado);
    static double getCostoEstado(Estado estado);
    static bool puedeGenerarEnergia(Estado estado);
    
    // Configuración
    void setMostrarDetalles(bool mostrar);
    
    // Gestión de caché de patrones
    void limpiarCachePatrones();
    int getTamañoCachePatrones() const;
    void mostrarEstadisticasCache() const;

private:
    // Algoritmo recursivo principal (versión original)
    EstadoMemo resolverRecursivo(int hora, Estado estadoLlegada, const Escenario& escenario);
    
    // Algoritmo recursivo con caché integrado
    EstadoMemo resolverRecursivoConCache(int hora, Estado estadoLlegada, const Escenario& escenario);
    
    // Sistema de detección y manejo de patrones
    std::pair<std::vector<int>, int> detectarPatronCompleto(const std::vector<int>& datos, int horaInicio);
    void extraerNuevosPatrones(const std::vector<int>& datos, const std::vector<Estado>& solucion);
    
    // Aplicación de caché existente
    bool aplicarCacheExistente(const Escenario& escenario);
    void prememoizarPatron(const std::vector<int>& patron, int horaInicio, int horaFin, const Escenario& escenario);
    
    // Caché de patrones
    bool estaEnCache(const std::vector<int>& patron) const;
    std::vector<Estado> obtenerDeCache(const std::vector<int>& patron) const;
    void guardarEnCache(const std::vector<int>& patron, const std::vector<Estado>& solucion);
    
    // Validación de transiciones
    std::vector<Estado> getEstadosQueVanA(Estado estadoDestino) const;
    bool esTransicionValida(Estado desde, Estado hacia) const;
    
    // Reconstrucción de la solución
    std::vector<Estado> reconstruirSolucion(const Escenario& escenario, int horaInicial);
    
    // Utilidades privadas
    void limpiarMemoizacion();
    void inicializarTransiciones();
    std::string patronAString(const std::vector<int>& patron) const;
    
    // Métodos obsoletos (mantenidos por compatibilidad)
    std::vector<SubsecuenciaInfo> identificarSubsecuencias(const std::vector<int>& datos);
    std::vector<Estado> resolverSubsecuencia(const std::vector<int>& patron);
    void extraerYGuardarPatrones(const std::vector<int>& patronOriginal, const std::vector<Estado>& solucion);
    std::vector<Estado> ensamblarSolucionCompleta(const std::vector<SubsecuenciaInfo>& subsecuencias, 
                                                  const std::vector<int>& datosOriginales);
    
    // Miembros privados
    std::map<std::pair<int, Estado>, EstadoMemo> memo;
    std::map<Estado, std::vector<Estado>> transicionesValidas;
    std::map<std::vector<int>, std::vector<Estado>> cachePatrones;  // Caché de patrones resueltos
    bool mostrarDetalles;
    int horaInicialActual;
    
    // Estadísticas
    mutable int cachehits;
    mutable int cachemisses;
}; 