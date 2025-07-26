#ifndef CALCULADOR_COSTOS_HPP
#define CALCULADOR_COSTOS_HPP

#include "escenario.hpp"
#include <vector>
#include <map>
#include <memory>

// Enumeración para los estados de la máquina
enum class EstadoMaquina {
    ON_CALIENTE,
    OFF_CALIENTE,
    ON_TIBIO,
    OFF_TIBIO,
    ON_FRIO,
    OFF_FRIO
};

// Estructura para representar una solución
struct Solucion {
    std::vector<EstadoMaquina> estados_por_hora;  // Estados para horas 0-23
    double costo_total;
    bool es_valida;
    
    Solucion() : estados_por_hora(24, EstadoMaquina::OFF_FRIO), costo_total(0.0), es_valida(false) {}
};

// Estructura para memoización que incluye el mejor estado anterior
struct ResultadoMemo {
    double costo;
    bool es_valido;
    EstadoMaquina mejor_estado_anterior;
    
    ResultadoMemo() : costo(std::numeric_limits<double>::infinity()), es_valido(false), mejor_estado_anterior(EstadoMaquina::OFF_FRIO) {}
    ResultadoMemo(double c, bool v, EstadoMaquina e) : costo(c), es_valido(v), mejor_estado_anterior(e) {}
};

class CalculadorCostos {
private:
    const Escenario& escenario_;
    
    // Costos de mantener cada estado ON
    std::map<EstadoMaquina, double> costos_mantenimiento_;
    
    // Memoización mejorada para reconstruir soluciones
    std::map<std::pair<int, EstadoMaquina>, ResultadoMemo> memo_;
    
    // Métodos auxiliares
    std::vector<EstadoMaquina> obtenerTransicionesPosibles(EstadoMaquina estado_actual) const;
    std::vector<EstadoMaquina> obtenerEstadosQueVanA(EstadoMaquina estado_destino) const;
    double getCostoMantenimiento(EstadoMaquina estado) const;
    bool generaEnergia(EstadoMaquina estado) const;
    std::string estadoToString(EstadoMaquina estado) const;
    
    // Método recursivo principal mejorado
    ResultadoMemo resolver_recursivo(int hora, EstadoMaquina estado_llegada);
    
    // Reconstruir la solución usando backtracking
    void reconstruirSolucion(Solucion& solucion, EstadoMaquina estado_inicial);
    
public:
    // Constructor
    CalculadorCostos(const Escenario& escenario);
    
    // Configurar costos de mantenimiento
    void configurarCostos(double costo_frio, double costo_tibio, double costo_caliente);
    
    // Resolver el problema principal
    Solucion resolver();
    
    // Métodos de utilidad
    void mostrarSolucion(const Solucion& solucion) const;
    void limpiarMemoizacion();
    void mostrarAnalisisDetallado() const;
};

#endif // CALCULADOR_COSTOS_HPP
