#include "../include/optimizador.hpp"
#include <iostream>
#include <limits>
#include <algorithm>
#include <iomanip>

OptimizadorMaquina::OptimizadorMaquina(const std::vector<int>& disponibilidad_eolica)
    : energia_eolica(disponibilidad_eolica) {
}

Solucion OptimizadorMaquina::resolver() {
    if (energia_eolica.empty()) {
        return Solucion(0.0, false);
    }
    
    return resolverDesdeHora(energia_eolica.size() - 1);
}

Solucion OptimizadorMaquina::resolverDesdeHora(int hora_inicial) {
    limpiarCache();
    
    if (hora_inicial < 0 || hora_inicial >= static_cast<int>(energia_eolica.size())) {
        return Solucion(0.0, false);
    }
    
    if (!esProblemaFactible()) {
        return Solucion(0.0, false);
    }
    
    // Probar todos los estados posibles para la hora final
    Solucion mejor_solucion;
    mejor_solucion.es_factible = false;
    mejor_solucion.costo_total = std::numeric_limits<double>::infinity();
    
    auto todos_estados = GestorEstados::obtenerTodosLosEstados();
    
    for (auto estado_final : todos_estados) {
        // Verificar si este estado final es válido para la hora inicial
        if (energia_eolica[hora_inicial] == 0 && !GestorEstados::puedeGenerarEnergia(estado_final)) {
            continue; // Este estado no puede satisfacer la demanda
        }
        
        auto solucion_actual = resolverRecursivo(hora_inicial, estado_final);
        
        if (solucion_actual.es_factible && 
            solucion_actual.costo_total < mejor_solucion.costo_total) {
            mejor_solucion = solucion_actual;
        }
    }
    
    if (mejor_solucion.es_factible) {
        reconstruirSolucion(mejor_solucion, hora_inicial);
    }
    
    return mejor_solucion;
}

Solucion OptimizadorMaquina::resolverRecursivo(int hora, EstadoMaquina estado_llegada) {
    // Caso base: llegamos antes de la hora 0
    if (hora < 0) {
        return Solucion(0.0, true);
    }
    
    // Verificar cache de memoización
    EstadoRecursion estado_key = {hora, estado_llegada};
    auto it = memo.find(estado_key);
    if (it != memo.end()) {
        return it->second;
    }
    
    // Verificar si el estado actual puede satisfacer la demanda de esta hora
    if (energia_eolica[hora] == 0 && !GestorEstados::puedeGenerarEnergia(estado_llegada)) {
        // No se puede satisfacer la demanda con este estado
        Solucion solucion_invalida(std::numeric_limits<double>::infinity(), false);
        memo[estado_key] = solucion_invalida;
        return solucion_invalida;
    }
    
    // Obtener estados desde los cuales se puede llegar al estado actual
    auto estados_origen = GestorEstados::obtenerEstadosOrigen(estado_llegada);
    
    Solucion mejor_solucion;
    mejor_solucion.es_factible = false;
    mejor_solucion.costo_total = std::numeric_limits<double>::infinity();
    
    // Probar cada estado origen posible
    for (auto estado_origen : estados_origen) {
        auto solucion_anterior = resolverRecursivo(hora - 1, estado_origen);
        
        if (solucion_anterior.es_factible) {
            double costo_total = solucion_anterior.costo_total + 
                               GestorEstados::obtenerCosto(estado_llegada);
            
            if (costo_total < mejor_solucion.costo_total) {
                mejor_solucion.costo_total = costo_total;
                mejor_solucion.es_factible = true;
            }
        }
    }
    
    // Si no se encontró ninguna solución válida desde estados origen,
    // verificar si se puede empezar desde este estado en la hora actual
    if (!mejor_solucion.es_factible && hora == 0) {
        mejor_solucion.costo_total = GestorEstados::obtenerCosto(estado_llegada);
        mejor_solucion.es_factible = true;
    }
    
    // Guardar en cache y retornar
    memo[estado_key] = mejor_solucion;
    return mejor_solucion;
}

void OptimizadorMaquina::reconstruirSolucion(Solucion& solucion, int hora_inicial) {
    solucion.secuencia_estados.clear();
    solucion.secuencia_estados.resize(hora_inicial + 1);
    
    // Buscar hacia atrás la secuencia óptima que produce el costo mínimo
    // Empezar desde la hora inicial y trabajar hacia atrás
    
    // Encontrar todos los estados válidos para la hora inicial que dan el costo óptimo
    auto todos_estados = GestorEstados::obtenerTodosLosEstados();
    EstadoMaquina estado_final_optimo = EstadoMaquina::ON_CALIENTE;
    
    for (auto estado : todos_estados) {
        if (energia_eolica[hora_inicial] == 0 && !GestorEstados::puedeGenerarEnergia(estado)) {
            continue;
        }
        
        auto solucion_temp = resolverRecursivo(hora_inicial, estado);
        if (solucion_temp.es_factible && solucion_temp.costo_total == solucion.costo_total) {
            estado_final_optimo = estado;
            break;
        }
    }
    
    // Reconstruir hacia atrás
    std::vector<EstadoMaquina> secuencia_inversa;
    EstadoMaquina estado_actual = estado_final_optimo;
    
    for (int hora = hora_inicial; hora >= 0; hora--) {
        secuencia_inversa.push_back(estado_actual);
        
        if (hora == 0) break;
        
        // Encontrar el estado anterior óptimo
        auto estados_origen = GestorEstados::obtenerEstadosOrigen(estado_actual);
        EstadoMaquina mejor_anterior = estado_actual;
        double mejor_costo = std::numeric_limits<double>::infinity();
        
        for (auto anterior : estados_origen) {
            if (energia_eolica[hora - 1] == 0 && !GestorEstados::puedeGenerarEnergia(anterior)) {
                continue;
            }
            
            auto solucion_desde_anterior = resolverRecursivo(hora - 1, anterior);
            if (solucion_desde_anterior.es_factible) {
                double costo_hasta_anterior = solucion_desde_anterior.costo_total;
                
                if (costo_hasta_anterior < mejor_costo) {
                    mejor_costo = costo_hasta_anterior;
                    mejor_anterior = anterior;
                }
            }
        }
        
        estado_actual = mejor_anterior;
    }
    
    // Invertir la secuencia para obtener el orden correcto (hora 0 al final)
    std::reverse(secuencia_inversa.begin(), secuencia_inversa.end());
    solucion.secuencia_estados = secuencia_inversa;
}

void OptimizadorMaquina::mostrarEstadisticas() const {
    int horas_con_eolica = std::count(energia_eolica.begin(), energia_eolica.end(), 1);
    int horas_sin_eolica = energia_eolica.size() - horas_con_eolica;
    
    std::cout << "=== ESTADÍSTICAS DEL PROBLEMA ===" << std::endl;
    std::cout << "Horizonte temporal: " << energia_eolica.size() << " horas" << std::endl;
    std::cout << "Horas con energía eólica suficiente: " << horas_con_eolica 
              << "/" << energia_eolica.size() << std::endl;
    std::cout << "Horas que requieren generación: " << horas_sin_eolica 
              << "/" << energia_eolica.size() << std::endl;
    std::cout << "Tipo de escenario: " << obtenerTipoEscenario() << std::endl;
    std::cout << "Estados en caché: " << memo.size() << std::endl;
}

void OptimizadorMaquina::limpiarCache() {
    memo.clear();
}

bool OptimizadorMaquina::esProblemaFactible() const {
    // El problema siempre es factible si existe al menos un estado que puede generar energía
    return true;  // ON_CALIENTE siempre puede generar energía
}

std::string OptimizadorMaquina::obtenerTipoEscenario() const {
    int horas_con_eolica = std::count(energia_eolica.begin(), energia_eolica.end(), 1);
    int total_horas = energia_eolica.size();
    
    if (horas_con_eolica == 0) {
        return "⚠️  CRÍTICO: Sin energía eólica";
    } else if (horas_con_eolica == total_horas) {
        return "✅ ÓPTIMO: Energía eólica completa";
    } else {
        return "⚖️  MIXTO: Optimización necesaria";
    }
} 