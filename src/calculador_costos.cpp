#include "calculador_costos.hpp"
#include <iostream>
#include <algorithm>
#include <climits>
#include <limits>

CalculadorCostos::CalculadorCostos(const Escenario& escenario) : escenario_(escenario) {
    // Inicializar costos por defecto
    costos_mantenimiento_[EstadoMaquina::ON_FRIO] = 1.0;
    costos_mantenimiento_[EstadoMaquina::ON_TIBIO] = 2.0;
    costos_mantenimiento_[EstadoMaquina::ON_CALIENTE] = 3.0;
    
    // Estados OFF no tienen costo
    costos_mantenimiento_[EstadoMaquina::OFF_FRIO] = 0.0;
    costos_mantenimiento_[EstadoMaquina::OFF_TIBIO] = 0.0;
    costos_mantenimiento_[EstadoMaquina::OFF_CALIENTE] = 0.0;
}

void CalculadorCostos::configurarCostos(double costo_frio, double costo_tibio, double costo_caliente) {
    costos_mantenimiento_[EstadoMaquina::ON_FRIO] = costo_frio;
    costos_mantenimiento_[EstadoMaquina::ON_TIBIO] = costo_tibio;
    costos_mantenimiento_[EstadoMaquina::ON_CALIENTE] = costo_caliente;
}

std::vector<EstadoMaquina> CalculadorCostos::obtenerTransicionesPosibles(EstadoMaquina estado_actual) const {
    std::vector<EstadoMaquina> transiciones;
    
    switch (estado_actual) {
        case EstadoMaquina::ON_CALIENTE:
            transiciones.push_back(EstadoMaquina::OFF_CALIENTE);
            transiciones.push_back(EstadoMaquina::ON_CALIENTE);  // a sí mismo
            break;
            
        case EstadoMaquina::OFF_CALIENTE:
            transiciones.push_back(EstadoMaquina::ON_TIBIO);
            transiciones.push_back(EstadoMaquina::OFF_TIBIO);
            break;
            
        case EstadoMaquina::ON_TIBIO:
            transiciones.push_back(EstadoMaquina::ON_CALIENTE);
            transiciones.push_back(EstadoMaquina::OFF_CALIENTE);
            break;
            
        case EstadoMaquina::OFF_TIBIO:
            transiciones.push_back(EstadoMaquina::ON_FRIO);
            transiciones.push_back(EstadoMaquina::OFF_FRIO);
            break;
            
        case EstadoMaquina::ON_FRIO:
            transiciones.push_back(EstadoMaquina::ON_TIBIO);
            transiciones.push_back(EstadoMaquina::OFF_TIBIO);
            break;
            
        case EstadoMaquina::OFF_FRIO:
            transiciones.push_back(EstadoMaquina::ON_FRIO);
            transiciones.push_back(EstadoMaquina::OFF_FRIO);  // a sí mismo
            break;
    }
    
    return transiciones;
}

std::vector<EstadoMaquina> CalculadorCostos::obtenerEstadosQueVanA(EstadoMaquina estado_destino) const {
    std::vector<EstadoMaquina> estados_origen;
    
    switch (estado_destino) {
        case EstadoMaquina::ON_CALIENTE:
            estados_origen.push_back(EstadoMaquina::ON_CALIENTE);  // de sí mismo
            estados_origen.push_back(EstadoMaquina::ON_TIBIO);
            break;
            
        case EstadoMaquina::OFF_CALIENTE:
            estados_origen.push_back(EstadoMaquina::ON_CALIENTE);
            estados_origen.push_back(EstadoMaquina::ON_TIBIO);
            break;
            
        case EstadoMaquina::ON_TIBIO:
            estados_origen.push_back(EstadoMaquina::OFF_CALIENTE);
            estados_origen.push_back(EstadoMaquina::ON_FRIO);
            break;
            
        case EstadoMaquina::OFF_TIBIO:
            estados_origen.push_back(EstadoMaquina::OFF_CALIENTE);
            estados_origen.push_back(EstadoMaquina::ON_FRIO);
            break;
            
        case EstadoMaquina::ON_FRIO:
            estados_origen.push_back(EstadoMaquina::OFF_TIBIO);
            estados_origen.push_back(EstadoMaquina::OFF_FRIO);
            break;
            
        case EstadoMaquina::OFF_FRIO:
            estados_origen.push_back(EstadoMaquina::OFF_TIBIO);
            estados_origen.push_back(EstadoMaquina::OFF_FRIO);  // de sí mismo
            break;
    }
    
    return estados_origen;
}

double CalculadorCostos::getCostoMantenimiento(EstadoMaquina estado) const {
    return costos_mantenimiento_.at(estado);
}

bool CalculadorCostos::generaEnergia(EstadoMaquina estado) const {
    return estado == EstadoMaquina::ON_CALIENTE;
}

std::string CalculadorCostos::estadoToString(EstadoMaquina estado) const {
    switch (estado) {
        case EstadoMaquina::ON_CALIENTE: return "ON/CALIENTE";
        case EstadoMaquina::OFF_CALIENTE: return "OFF/CALIENTE";
        case EstadoMaquina::ON_TIBIO: return "ON/TIBIO";
        case EstadoMaquina::OFF_TIBIO: return "OFF/TIBIO";
        case EstadoMaquina::ON_FRIO: return "ON/FRIO";
        case EstadoMaquina::OFF_FRIO: return "OFF/FRIO";
        default: return "DESCONOCIDO";
    }
}

ResultadoMemo CalculadorCostos::resolver_recursivo(int hora, EstadoMaquina estado_llegada) {
    // Caso base: hora -1 (antes de la hora 0)
    if (hora < 0) {
        return ResultadoMemo(0.0, true, EstadoMaquina::OFF_FRIO);
    }
    
    // Verificar memoización
    auto clave = std::make_pair(hora, estado_llegada);
    if (memo_.find(clave) != memo_.end()) {
        return memo_[clave];
    }
    
    double mejor_costo = std::numeric_limits<double>::infinity();
    bool solucion_encontrada = false;
    EstadoMaquina mejor_estado_anterior = EstadoMaquina::OFF_FRIO;
    
    // Verificar si la demanda se cubre con energía de otras fuentes
    bool demanda_cubierta = escenario_.demandaCubiertaConEO(hora);
    
    if (demanda_cubierta) {
        // La demanda se cubre con EO, podemos explorar diferentes estados
        std::vector<EstadoMaquina> estados_posibles = obtenerEstadosQueVanA(estado_llegada);
        
        for (EstadoMaquina estado_actual : estados_posibles) {
            double costo_actual = getCostoMantenimiento(estado_actual);
            
            ResultadoMemo resultado = resolver_recursivo(hora - 1, estado_actual);
            if (resultado.es_valido) {  // si hay solución válida
                double costo_total = costo_actual + resultado.costo;
                if (costo_total < mejor_costo) {
                    mejor_costo = costo_total;
                    solucion_encontrada = true;
                    mejor_estado_anterior = estado_actual;
                }
            }
        }
    } else {
        // La demanda NO se cubre con EO, necesitamos ON/CALIENTE
        std::vector<EstadoMaquina> estados_posibles = obtenerEstadosQueVanA(estado_llegada);
        
        // Verificar si alguno de los estados posibles puede generar energía
        bool hay_estado_valido = false;
        for (EstadoMaquina estado : estados_posibles) {
            if (generaEnergia(estado)) {
                hay_estado_valido = true;
                break;
            }
        }
        
        if (!hay_estado_valido) {
            // No hay transición válida que permita generar energía
            ResultadoMemo resultado_invalido;
            memo_[clave] = resultado_invalido;
            return resultado_invalido;
        }
        
        // Solo consideramos ON/CALIENTE
        for (EstadoMaquina estado_actual : estados_posibles) {
            if (generaEnergia(estado_actual)) {
                double costo_actual = getCostoMantenimiento(estado_actual);
                
                ResultadoMemo resultado = resolver_recursivo(hora - 1, estado_actual);
                if (resultado.es_valido) {  // si hay solución válida
                    double costo_total = costo_actual + resultado.costo;
                    if (costo_total < mejor_costo) {
                        mejor_costo = costo_total;
                        solucion_encontrada = true;
                        mejor_estado_anterior = estado_actual;
                    }
                }
            }
        }
    }
    
    ResultadoMemo resultado_final(mejor_costo, solucion_encontrada, mejor_estado_anterior);
    memo_[clave] = resultado_final;
    return resultado_final;
}

void CalculadorCostos::reconstruirSolucion(Solucion& solucion, EstadoMaquina estado_inicial) {
    solucion.estados_por_hora[23] = estado_inicial;
    
    // Reconstruir desde la hora 22 hacia atrás
    for (int hora = 22; hora >= 0; hora--) {
        EstadoMaquina estado_siguiente = solucion.estados_por_hora[hora + 1];
        auto clave = std::make_pair(hora, estado_siguiente);
        
        if (memo_.find(clave) != memo_.end()) {
            solucion.estados_por_hora[hora] = memo_[clave].mejor_estado_anterior;
        } else {
            // Error en la reconstrucción
            solucion.es_valida = false;
            return;
        }
    }
}

Solucion CalculadorCostos::resolver() {
    Solucion mejor_solucion;
    mejor_solucion.costo_total = std::numeric_limits<double>::infinity();
    
    std::cout << "\n=== INICIANDO RESOLUCIÓN DESDE HORA 23 ===" << std::endl;
    
    // Verificar si la demanda de la hora 23 se cubre con EO
    bool demanda_23_cubierta = escenario_.demandaCubiertaConEO(23);
    std::cout << "Demanda hora 23 cubierta con EO: " << (demanda_23_cubierta ? "Sí" : "No") << std::endl;
    
    std::vector<EstadoMaquina> estados_iniciales;
    
    if (demanda_23_cubierta) {
        // Puede estar en cualquier estado OFF
        estados_iniciales = {EstadoMaquina::OFF_FRIO, EstadoMaquina::OFF_TIBIO, EstadoMaquina::OFF_CALIENTE};
        std::cout << "Explorando estados OFF posibles para hora 23..." << std::endl;
    } else {
        // Debe estar en ON/CALIENTE
        estados_iniciales = {EstadoMaquina::ON_CALIENTE};
        std::cout << "Hora 23 debe estar en ON/CALIENTE" << std::endl;
    }
    
    EstadoMaquina mejor_estado_inicial = EstadoMaquina::OFF_FRIO;
    
    for (EstadoMaquina estado_23 : estados_iniciales) {
        std::cout << "\nProbando estado inicial: " << estadoToString(estado_23) << std::endl;
        
        // Limpiar memoización para cada intento
        limpiarMemoizacion();
        
        double costo_23 = getCostoMantenimiento(estado_23);
        ResultadoMemo resultado = resolver_recursivo(22, estado_23);
        
        if (resultado.es_valido) {  // si hay solución válida
            double costo_total = costo_23 + resultado.costo;
            std::cout << "Costo encontrado: " << costo_total << std::endl;
            
            if (costo_total < mejor_solucion.costo_total) {
                mejor_solucion.costo_total = costo_total;
                mejor_solucion.es_valida = true;
                mejor_estado_inicial = estado_23;
                
                std::cout << "¡Nueva mejor solución encontrada!" << std::endl;
            }
        } else {
            std::cout << "No se encontró solución válida desde este estado" << std::endl;
        }
    }
    
    // Reconstruir la solución completa usando el mejor estado inicial encontrado
    if (mejor_solucion.es_valida) {
        // Volver a ejecutar la recursión con el mejor estado para tener la memoización correcta
        limpiarMemoizacion();
        resolver_recursivo(22, mejor_estado_inicial);
        
        // Ahora reconstruir la solución completa
        reconstruirSolucion(mejor_solucion, mejor_estado_inicial);
    }
    
    return mejor_solucion;
}

void CalculadorCostos::mostrarSolucion(const Solucion& solucion) const {
    std::cout << "\n=== SOLUCIÓN ENCONTRADA ===" << std::endl;
    
    if (!solucion.es_valida) {
        std::cout << "No se encontró una solución válida." << std::endl;
        return;
    }
    
    std::cout << "Costo total: " << solucion.costo_total << std::endl;
    std::cout << "\nEstados por hora:" << std::endl;
    std::cout << "Hora\tEstado\t\tCosto\tDemanda\tEO\tCubierta" << std::endl;
    std::cout << "----\t------\t\t-----\t-------\t--\t--------" << std::endl;
    
    for (int hora = 0; hora < 24; hora++) {
        EstadoMaquina estado = solucion.estados_por_hora[hora];
        double costo = getCostoMantenimiento(estado);
        double demanda = escenario_.getDemanda(hora);
        double eo = escenario_.getEnergiaOtrasFuentes(hora);
        bool cubierta = escenario_.demandaCubiertaConEO(hora);
        
        std::cout << hora << "\t" << estadoToString(estado) << "\t" 
                  << costo << "\t" << demanda << "\t" << eo << "\t"
                  << (cubierta ? "Sí" : "No") << std::endl;
    }
}

void CalculadorCostos::limpiarMemoizacion() {
    memo_.clear();
}

void CalculadorCostos::mostrarAnalisisDetallado() const {
    std::cout << "\n=== ANÁLISIS DETALLADO DEL ESCENARIO ===" << std::endl;
    
    int horas_sin_cobertura = 0;
    int horas_con_cobertura = 0;
    
    for (int hora = 0; hora < 24; hora++) {
        bool cubierta = escenario_.demandaCubiertaConEO(hora);
        if (cubierta) {
            horas_con_cobertura++;
        } else {
            horas_sin_cobertura++;
        }
    }
    
    std::cout << "Horas con demanda cubierta por EO: " << horas_con_cobertura << "/24" << std::endl;
    std::cout << "Horas que requieren generación: " << horas_sin_cobertura << "/24" << std::endl;
    
    if (horas_sin_cobertura == 24) {
        std::cout << "\n⚠️  ESCENARIO CRÍTICO: Todas las horas requieren ON/CALIENTE" << std::endl;
        std::cout << "   Costo mínimo teórico: " << (24 * getCostoMantenimiento(EstadoMaquina::ON_CALIENTE)) << std::endl;
    } else if (horas_con_cobertura == 24) {
        std::cout << "\n✓ ESCENARIO ÓPTIMO: Toda la demanda se cubre con EO" << std::endl;
        std::cout << "   Puede usar solo estados OFF (costo = 0)" << std::endl;
    } else {
        std::cout << "\n⚖️  ESCENARIO MIXTO: Optimización necesaria" << std::endl;
        std::cout << "   Balance entre estados ON y transiciones válidas" << std::endl;
    }
}
