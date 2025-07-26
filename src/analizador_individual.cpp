#include "calculador_costos.hpp"
#include "escenario.hpp"
#include <iostream>
#include <bitset>
#include <iomanip>

// Función para convertir patrón binario string a bitset
std::bitset<24> stringToBitset(const std::string& patron) {
    if (patron.length() != 24) {
        throw std::invalid_argument("El patrón debe tener exactamente 24 bits");
    }
    
    std::bitset<24> resultado;
    for (int i = 0; i < 24; i++) {
        if (patron[i] == '1') {
            resultado[23-i] = 1;  // Invertir orden para que hora 0 sea bit izquierdo
        } else if (patron[i] != '0') {
            throw std::invalid_argument("El patrón debe contener solo 0s y 1s");
        }
    }
    return resultado;
}

// Función para convertir estado a string
std::string estadoToString(EstadoMaquina estado) {
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

// Función para obtener costo de mantenimiento
double getCostoMantenimiento(EstadoMaquina estado) {
    switch (estado) {
        case EstadoMaquina::ON_FRIO: return 1.0;
        case EstadoMaquina::ON_TIBIO: return 2.5;
        case EstadoMaquina::ON_CALIENTE: return 5.0;
        default: return 0.0;  // Estados OFF no tienen costo
    }
}

// Función para verificar si genera energía
bool generaEnergia(EstadoMaquina estado) {
    return estado == EstadoMaquina::ON_CALIENTE;
}

// Función para obtener transiciones posibles
std::vector<EstadoMaquina> obtenerTransicionesPosibles(EstadoMaquina estado_actual) {
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

// Función para mostrar información detallada del escenario
void mostrarAnalisisDetallado(const Escenario& escenario, const std::bitset<24>& patron_eolica) {
    std::cout << "\n=== ANÁLISIS DETALLADO DEL ESCENARIO ===\n";
    std::cout << "Patrón de energía eólica: " << patron_eolica << "\n";
    std::cout << "Interpretación (horas con eólica): ";
    for (int i = 0; i < 24; i++) {
        if (patron_eolica[23-i]) {  // Corregir orden de bits: hora 0 = bit 23
            std::cout << "H" << i << " ";
        }
    }
    std::cout << "\n\n";
    
    std::cout << std::setw(4) << "Hora" 
              << std::setw(10) << "Demanda" 
              << std::setw(8) << "Eólica" 
              << std::setw(10) << "Déficit" 
              << std::setw(12) << "¿Crítica?" << "\n";
    std::cout << std::string(50, '-') << "\n";
    
    int horas_criticas = 0;
    double deficit_total = 0;
    
    for (int hora = 0; hora < 24; hora++) {
        double demanda = escenario.getDemanda(hora);
        double eolica = escenario.getEnergiaOtrasFuentes(hora);
        double deficit = std::max(0.0, demanda - eolica);
        bool critica = !escenario.demandaCubiertaConEO(hora);
        
        if (critica) horas_criticas++;
        deficit_total += deficit;
        
        std::cout << std::setw(4) << hora 
                  << std::setw(10) << std::fixed << std::setprecision(0) << demanda
                  << std::setw(8) << eolica
                  << std::setw(10) << deficit
                  << std::setw(12) << (critica ? "SÍ" : "No") << "\n";
    }
    
    std::cout << std::string(50, '-') << "\n";
    std::cout << "RESUMEN:\n";
    std::cout << "- Horas críticas: " << horas_criticas << "/24\n";
    std::cout << "- Déficit total: " << std::fixed << std::setprecision(0) << deficit_total << " MW\n";
    std::cout << "- Cobertura eólica: " << std::fixed << std::setprecision(1) 
              << (24.0 - horas_criticas) / 24.0 * 100.0 << "%\n";
}

void mostrarProcesoOptimizacion(const Escenario& escenario) {
    std::cout << "\n=== PROCESO DE OPTIMIZACIÓN DETALLADO ===\n\n";
    
    // Verificar hora 23 primero
    bool demanda_23_cubierta = escenario.demandaCubiertaConEO(23);
    std::cout << "🎯 INICIANDO DESDE HORA 23:\n";
    std::cout << "   Demanda hora 23: " << escenario.getDemanda(23) << " MW\n";
    std::cout << "   Energía eólica: " << escenario.getEnergiaOtrasFuentes(23) << " MW\n";
    std::cout << "   ¿Cubierta por eólica? " << (demanda_23_cubierta ? "SÍ" : "NO") << "\n\n";
    
    if (demanda_23_cubierta) {
        std::cout << "✅ Como la demanda está cubierta, exploramos estados OFF posibles:\n";
        std::cout << "   - OFF/FRIO (costo: 0)\n";
        std::cout << "   - OFF/TIBIO (costo: 0)\n";
        std::cout << "   - OFF/CALIENTE (costo: 0)\n\n";
        std::cout << "🔍 El algoritmo evaluará cuál de estos lleva al menor costo total...\n\n";
    } else {
        std::cout << "⚡ Como la demanda NO está cubierta, OBLIGATORIAMENTE debe estar en:\n";
        std::cout << "   - ON/CALIENTE (costo: 5.0) - único estado que genera energía\n\n";
    }
}

void mostrarSolucionDetallada(const Solucion& solucion, const Escenario& escenario) {
    std::cout << "\n=== SOLUCIÓN ÓPTIMA ENCONTRADA ===\n";
    
    if (!solucion.es_valida) {
        std::cout << "❌ No se encontró solución válida\n";
        return;
    }
    
    std::cout << "💰 Costo total mínimo: " << std::fixed << std::setprecision(2) 
              << solucion.costo_total << "\n\n";
    
    std::cout << "📋 SECUENCIA COMPLETA DE ESTADOS:\n";
    std::cout << std::setw(4) << "Hora" 
              << std::setw(15) << "Estado" 
              << std::setw(8) << "Costo"
              << std::setw(10) << "Demanda"
              << std::setw(8) << "Eólica"
              << std::setw(12) << "¿Genera?"
              << std::setw(12) << "¿Cubierta?" << "\n";
    std::cout << std::string(75, '-') << "\n";
    
    double costo_acumulado = 0;
    for (int hora = 0; hora < 24; hora++) {
        EstadoMaquina estado = solucion.estados_por_hora[hora];
        double costo_hora = getCostoMantenimiento(estado);
        costo_acumulado += costo_hora;
        
        double demanda = escenario.getDemanda(hora);
        double eolica = escenario.getEnergiaOtrasFuentes(hora);
        bool genera = generaEnergia(estado);
        bool cubierta = escenario.demandaCubiertaConEO(hora) || genera;
        
        std::cout << std::setw(4) << hora 
                  << std::setw(15) << estadoToString(estado)
                  << std::setw(8) << std::fixed << std::setprecision(1) << costo_hora
                  << std::setw(10) << std::fixed << std::setprecision(0) << demanda
                  << std::setw(8) << eolica
                  << std::setw(12) << (genera ? "SÍ" : "No")
                  << std::setw(12) << (cubierta ? "SÍ" : "❌NO") << "\n";
    }
    
    std::cout << std::string(75, '-') << "\n";
    std::cout << "💰 Costo verificado: " << std::fixed << std::setprecision(2) << costo_acumulado << "\n\n";
    
    // Análisis de transiciones
    std::cout << "🔄 ANÁLISIS DE TRANSICIONES:\n";
    bool todas_validas = true;
    
    for (int hora = 1; hora < 24; hora++) {
        EstadoMaquina estado_anterior = solucion.estados_por_hora[hora-1];
        EstadoMaquina estado_actual = solucion.estados_por_hora[hora];
        
        if (estado_anterior != estado_actual) {
            std::cout << "   Hora " << hora-1 << " → " << hora << ": " 
                      << estadoToString(estado_anterior) << " → " 
                      << estadoToString(estado_actual);
            
            // Verificar si es una transición válida
            auto transiciones_validas = obtenerTransicionesPosibles(estado_anterior);
            bool es_valida = std::find(transiciones_validas.begin(), transiciones_validas.end(), estado_actual) 
                            != transiciones_validas.end();
            
            std::cout << (es_valida ? " ✅" : " ❌INVÁLIDA") << "\n";
            if (!es_valida) todas_validas = false;
        }
    }
    
    if (todas_validas) {
        std::cout << "✅ Todas las transiciones son válidas según la máquina de estados\n\n";
    } else {
        std::cout << "❌ Hay transiciones inválidas en la solución\n\n";
    }
    
    // Estadísticas
    std::cout << "📊 ESTADÍSTICAS DE LA SOLUCIÓN:\n";
    
    int count_on_caliente = 0, count_on_tibio = 0, count_on_frio = 0;
    int count_off_caliente = 0, count_off_tibio = 0, count_off_frio = 0;
    
    for (int hora = 0; hora < 24; hora++) {
        switch (solucion.estados_por_hora[hora]) {
            case EstadoMaquina::ON_CALIENTE: count_on_caliente++; break;
            case EstadoMaquina::ON_TIBIO: count_on_tibio++; break;
            case EstadoMaquina::ON_FRIO: count_on_frio++; break;
            case EstadoMaquina::OFF_CALIENTE: count_off_caliente++; break;
            case EstadoMaquina::OFF_TIBIO: count_off_tibio++; break;
            case EstadoMaquina::OFF_FRIO: count_off_frio++; break;
        }
    }
    
    std::cout << "   Estados ON:  CALIENTE=" << count_on_caliente 
              << ", TIBIO=" << count_on_tibio 
              << ", FRIO=" << count_on_frio << "\n";
    std::cout << "   Estados OFF: CALIENTE=" << count_off_caliente 
              << ", TIBIO=" << count_off_tibio 
              << ", FRIO=" << count_off_frio << "\n";
    
    int horas_generando = count_on_caliente;
    int horas_consumiendo = count_on_caliente + count_on_tibio + count_on_frio;
    
    std::cout << "   Horas generando energía: " << horas_generando << "/24\n";
    std::cout << "   Horas con costo operativo: " << horas_consumiendo << "/24\n";
    if (horas_consumiendo > 0) {
        std::cout << "   Eficiencia energética: " << std::fixed << std::setprecision(1)
                  << (double)horas_generando / horas_consumiendo * 100.0 << "%\n";
    }
}

int main() {
    std::cout << "=== ANALIZADOR DE CASOS INDIVIDUALES ===\n";
    std::cout << "Permite analizar en detalle un patrón específico de energía eólica\n\n";
    
    // Configurar demanda fija
    std::vector<double> demanda_fija = {
        300, 200, 100, 100, 100, 200, 300, 500, 800, 1000, 1000, 1000,
        1000, 900, 800, 800, 800, 1000, 1000, 1000, 600, 600, 400, 300
    };
    
    std::string patron_input;
    std::cout << "Ingresa el patrón de energía eólica (24 bits):\n";
    std::cout << "Formato: hora 0 = bit izquierdo, 1 = hay eólica (500 MW), 0 = sin eólica\n";
    std::cout << "Ejemplo: 000000000000000000001110 (eólica en horas 1,2,3)\n";
    std::cout << "Patrón: ";
    std::cin >> patron_input;
    
    try {
        // Convertir string a bitset
        std::bitset<24> patron_eolica = stringToBitset(patron_input);
        
        // Configurar escenario
        Escenario escenario;
        for (int hora = 0; hora < 24; hora++) {
            escenario.setDemanda(hora, demanda_fija[hora]);
            double energia_eolica = patron_eolica[23-hora] ? 500.0 : 0.0;  // Corregir orden de bits
            escenario.setEnergiaOtrasFuentes(hora, energia_eolica);
        }
        
        // Mostrar análisis detallado del escenario
        mostrarAnalisisDetallado(escenario, patron_eolica);
        
        // Mostrar proceso de optimización
        mostrarProcesoOptimizacion(escenario);
        
        // Resolver con el calculador normal
        CalculadorCostos calculador(escenario);
        calculador.configurarCostos(1.0, 2.5, 5.0);
        Solucion solucion = calculador.resolver();
        
        // Mostrar solución detallada
        mostrarSolucionDetallada(solucion, escenario);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cout << "\nEjemplos de uso correcto:\n";
        std::cout << "000000000000000000001110  (eólica en horas 1, 2, 3)\n";
        std::cout << "111111000000000000000000  (eólica en horas 0-5)\n";
        std::cout << "000000111111111111000000  (eólica en horas 6-17)\n";
        return 1;
    }
    
    return 0;
}
