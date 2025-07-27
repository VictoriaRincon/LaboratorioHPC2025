#include "analizador_exhaustivo.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <limits>

AnalizadorExhaustivo::AnalizadorExhaustivo() : 
    intervalo_reporte_(1000), 
    guardar_todas_soluciones_(false),
    umbral_costo_interes_(std::numeric_limits<double>::infinity()) {
    
    // Configurar demanda por defecto
    demanda_fija_ = {300, 200, 100, 100, 100, 200, 300, 500, 800, 1000, 1000, 1000, 
                     1000, 900, 800, 800, 800, 1000, 1000, 1000, 600, 600, 400, 300};
}

void AnalizadorExhaustivo::configurarDemanda(const std::vector<double>& demanda) {
    if (demanda.size() != 24) {
        throw std::invalid_argument("La demanda debe tener exactamente 24 valores");
    }
    demanda_fija_ = demanda;
}

void AnalizadorExhaustivo::configurarArchivos(const std::string& archivo_resultados, const std::string& archivo_log) {
    archivo_resultados_.open(archivo_resultados);
    archivo_log_.open(archivo_log);
    
    if (!archivo_resultados_.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo de resultados: " + archivo_resultados);
    }
    
    if (!archivo_log_.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo de log: " + archivo_log);
    }
    
    // Escribir headers
    archivo_resultados_ << "CombinacionID,PatronEolica,CostoTotal,SolucionValida,HorasCriticas,SecuenciaEstados\n";
    archivo_log_ << "=== ANÁLISIS EXHAUSTIVO DE MÁQUINA DE ESTADOS ===\n";
    archivo_log_ << "Inicio del análisis: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\n";
    archivo_log_ << "Total de combinaciones a procesar: " << stats_.combinaciones_totales << "\n\n";
}

void AnalizadorExhaustivo::configurarReporte(uint32_t intervalo, bool guardar_todas, double umbral_costo) {
    intervalo_reporte_ = intervalo;

std::string AnalizadorExhaustivo::estadoToString(EstadoMaquina estado) const {
    switch (estado) {
        case EstadoMaquina::ON_CALIENTE: return "ON_CALIENTE";
        case EstadoMaquina::OFF_CALIENTE: return "OFF_CALIENTE";
        case EstadoMaquina::ON_TIBIO: return "ON_TIBIO";
        case EstadoMaquina::OFF_TIBIO: return "OFF_TIBIO";
        case EstadoMaquina::ON_FRIO: return "ON_FRIO";
        case EstadoMaquina::OFF_FRIO: return "OFF_FRIO";
        default: return "DESCONOCIDO";
    }
}

void AnalizadorExhaustivo::guardarResultado(const ResultadoCombinacion& resultado) {
    // Solo guardar si cumple criterios
    if (!guardar_todas_soluciones_ && resultado.costo_total > umbral_costo_interes_) {
        return;
    }
    
    archivo_resultados_ << resultado.combinacion_id << ","
                       << resultado.patron_eolica << ","
                       << std::fixed << std::setprecision(2) << resultado.costo_total << ","
                       << (resultado.solucion_valida ? "SI" : "NO") << ","
                       << resultado.horas_criticas << ",";
    
    // Guardar secuencia de estados
    for (size_t i = 0; i < resultado.secuencia_optima.size(); i++) {
        if (i > 0) archivo_resultados_ << "-";
        archivo_resultados_ << estadoToString(resultado.secuencia_optima[i]);
    }
    archivo_resultados_ << "\n";
    
    archivo_resultados_.flush(); // Asegurar escritura
}

std::string AnalizadorExhaustivo::tiempoTranscurrido() const {
    auto ahora = std::chrono::steady_clock::now();
    auto duracion = std::chrono::duration_cast<std::chrono::seconds>(ahora - stats_.tiempo_inicio);
    
    int horas = duracion.count() / 3600;
    int minutos = (duracion.count() % 3600) / 60;
    int segundos = duracion.count() % 60;
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << horas << ":"
        << std::setfill('0') << std::setw(2) << minutos << ":"
        << std::setfill('0') << std::setw(2) << segundos;
    return oss.str();
}

double AnalizadorExhaustivo::tiempoEstimadoRestante() const {
    if (stats_.combinaciones_procesadas == 0) return 0.0;
    
    auto tiempo_transcurrido = std::chrono::steady_clock::now() - stats_.tiempo_inicio;
    double segundos_transcurridos = std::chrono::duration<double>(tiempo_transcurrido).count();
    
    double tasa_procesamiento = stats_.combinaciones_procesadas / segundos_transcurridos;
    uint32_t restantes = stats_.combinaciones_totales - stats_.combinaciones_procesadas;
    
    return restantes / tasa_procesamiento;
}

void AnalizadorExhaustivo::mostrarProgreso() {
    double porcentaje = (double)stats_.combinaciones_procesadas / stats_.combinaciones_totales * 100.0;
    double tiempo_restante = tiempoEstimadoRestante();
    
    std::cout << "\r[" << std::fixed << std::setprecision(2) << porcentaje << "%] "
              << stats_.combinaciones_procesadas << "/" << stats_.combinaciones_totales
              << " | Válidas: " << stats_.soluciones_validas
              << " | Tiempo: " << tiempoTranscurrido()
              << " | ETA: " << (int)(tiempo_restante/3600) << "h" << (int)((int)tiempo_restante%3600)/60 << "m"
              << std::flush;
}

void AnalizadorExhaustivo::generarReporteProgreso() {
    archivo_log_ << "PROGRESO [" << tiempoTranscurrido() << "] "
                << stats_.combinaciones_procesadas << "/" << stats_.combinaciones_totales
                << " (" << std::fixed << std::setprecision(2) 
                << (double)stats_.combinaciones_procesadas / stats_.combinaciones_totales * 100.0 << "%)\n";
    archivo_log_ << "  Soluciones válidas: " << stats_.soluciones_validas << "\n";
    archivo_log_ << "  Costo mínimo encontrado: " << stats_.costo_minimo_global << "\n";
    archivo_log_ << "  Costo máximo encontrado: " << stats_.costo_maximo_global << "\n";
    archivo_log_ << "  Costo promedio: " << stats_.costo_promedio << "\n\n";
    archivo_log_.flush();
}

void AnalizadorExhaustivo::configurarEscenario(Escenario& escenario, const std::bitset<24>& patron_eolica) {
    // Crear vectores para la configuración
    std::vector<double> energia_eolica(24);
    
    for (int hora = 0; hora < 24; hora++) {
        energia_eolica[hora] = patron_eolica[hora] ? 500.0 : 0.0;
    }
    
    // Configurar el escenario directamente
    escenario.configurarDirecto(demanda_fija_, energia_eolica);
}

void AnalizadorExhaustivo::ejecutarAnalisisCompleto() {
    std::cout << "\n=== INICIANDO ANÁLISIS EXHAUSTIVO ===\n";
    std::cout << "Total de combinaciones: " << stats_.combinaciones_totales << "\n";
    std::cout << "Esto puede tomar varios días de procesamiento...\n\n";
    
    stats_.tiempo_inicio = std::chrono::steady_clock::now();
    stats_.ultimo_reporte = stats_.tiempo_inicio;
    
    double suma_costos = 0.0;
    
    for (uint32_t combinacion = 0; combinacion < stats_.combinaciones_totales; combinacion++) {
        // Convertir número a patrón binario de 24 bits
        std::bitset<24> patron_eolica(combinacion);
        
        // Configurar escenario para esta combinación
        Escenario escenario;
        configurarEscenario(escenario, patron_eolica);
        
        // Crear calculador y resolver
        CalculadorCostos calculador(escenario);
        calculador.configurarCostos(1.0, 2.5, 5.0);
        
        Solucion solucion = calculador.resolver();
        
        // Almacenar resultado
        ResultadoCombinacion resultado;
        resultado.combinacion_id = combinacion;
        resultado.patron_eolica = patron_eolica;
        resultado.costo_total = solucion.costo_total;
        resultado.solucion_valida = solucion.es_valida;
        resultado.secuencia_optima = solucion.estados_por_hora;
        
        // Calcular horas críticas
        resultado.horas_criticas = 0;
        for (int hora = 0; hora < 24; hora++) {
            if (!escenario.demandaCubiertaConEO(hora)) {
                resultado.horas_criticas++;
            }
        }
        
        // Actualizar estadísticas
        stats_.combinaciones_procesadas++;
        if (solucion.es_valida) {
            stats_.soluciones_validas++;
            suma_costos += solucion.costo_total;
            
            if (solucion.costo_total < stats_.costo_minimo_global) {
                stats_.costo_minimo_global = solucion.costo_total;
            }
            if (solucion.costo_total > stats_.costo_maximo_global) {
                stats_.costo_maximo_global = solucion.costo_total;
            }
            
            stats_.costo_promedio = suma_costos / stats_.soluciones_validas;
        }
        
        // Guardar resultado
        guardarResultado(resultado);
        
        // Mostrar progreso
        if (stats_.combinaciones_procesadas % intervalo_reporte_ == 0) {
            mostrarProgreso();
            generarReporteProgreso();
        }
    }
    
    std::cout << "\n\n=== ANÁLISIS COMPLETADO ===\n";
    mostrarEstadisticasFinales();
    generarResumenEjecutivo();
}

void AnalizadorExhaustivo::ejecutarAnalisisParcial(uint32_t desde, uint32_t hasta) {
    if (hasta > stats_.combinaciones_totales) {
        hasta = stats_.combinaciones_totales;
    }
    
    std::cout << "\n=== ANÁLISIS PARCIAL [" << desde << " - " << hasta << "] ===\n";
    
    stats_.tiempo_inicio = std::chrono::steady_clock::now();
    stats_.ultimo_reporte = stats_.tiempo_inicio;
    stats_.combinaciones_totales = hasta - desde;
    
    double suma_costos = 0.0;
    
    for (uint32_t combinacion = desde; combinacion < hasta; combinacion++) {
        std::bitset<24> patron_eolica(combinacion);
        
        Escenario escenario;
        configurarEscenario(escenario, patron_eolica);
        
        CalculadorCostos calculador(escenario);
        calculador.configurarCostos(1.0, 2.5, 5.0);
        
        Solucion solucion = calculador.resolver();
        
        ResultadoCombinacion resultado;
        resultado.combinacion_id = combinacion;
        resultado.patron_eolica = patron_eolica;
        resultado.costo_total = solucion.costo_total;
        resultado.solucion_valida = solucion.es_valida;
        resultado.secuencia_optima = solucion.estados_por_hora;
        
        resultado.horas_criticas = 0;
        for (int hora = 0; hora < 24; hora++) {
            if (!escenario.demandaCubiertaConEO(hora)) {
                resultado.horas_criticas++;
            }
        }
        
        stats_.combinaciones_procesadas++;
        if (solucion.es_valida) {
            stats_.soluciones_validas++;
            suma_costos += solucion.costo_total;
            
            if (solucion.costo_total < stats_.costo_minimo_global) {
                stats_.costo_minimo_global = solucion.costo_total;
            }
            if (solucion.costo_total > stats_.costo_maximo_global) {
                stats_.costo_maximo_global = solucion.costo_total;
            }
            
            stats_.costo_promedio = suma_costos / stats_.soluciones_validas;
        }
        
        guardarResultado(resultado);
        
        if (stats_.combinaciones_procesadas % intervalo_reporte_ == 0) {
            mostrarProgreso();
            generarReporteProgreso();
        }
    }
    
    std::cout << "\n\n=== ANÁLISIS PARCIAL COMPLETADO ===\n";
    mostrarEstadisticasFinales();
}

void AnalizadorExhaustivo::mostrarEstadisticasFinales() {
    std::cout << "\n=== ESTADÍSTICAS FINALES ===\n";
    std::cout << "Combinaciones procesadas: " << stats_.combinaciones_procesadas << "\n";
    std::cout << "Soluciones válidas: " << stats_.soluciones_validas 
              << " (" << std::fixed << std::setprecision(2) 
              << (double)stats_.soluciones_validas / stats_.combinaciones_procesadas * 100.0 << "%)\n";
    std::cout << "Costo mínimo encontrado: " << stats_.costo_minimo_global << "\n";
    std::cout << "Costo máximo encontrado: " << stats_.costo_maximo_global << "\n";
    std::cout << "Costo promedio: " << stats_.costo_promedio << "\n";
    std::cout << "Tiempo total: " << tiempoTranscurrido() << "\n";
    
    if (stats_.combinaciones_procesadas > 0) {
        auto tiempo_total = std::chrono::steady_clock::now() - stats_.tiempo_inicio;
        double segundos_totales = std::chrono::duration<double>(tiempo_total).count();
        double tasa = stats_.combinaciones_procesadas / segundos_totales;
        std::cout << "Tasa de procesamiento: " << std::fixed << std::setprecision(1) 
                  << tasa << " combinaciones/segundo\n";
    }
}

void AnalizadorExhaustivo::generarResumenEjecutivo() {
    archivo_log_ << "\n=== RESUMEN EJECUTIVO ===\n";
    archivo_log_ << "Análisis completado el: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\n";
    archivo_log_ << "Tiempo total de procesamiento: " << tiempoTranscurrido() << "\n";
    archivo_log_ << "Combinaciones procesadas: " << stats_.combinaciones_procesadas << "\n";
    archivo_log_ << "Soluciones válidas encontradas: " << stats_.soluciones_validas << "\n";
    archivo_log_ << "Tasa de éxito: " << std::fixed << std::setprecision(2) 
                << (double)stats_.soluciones_validas / stats_.combinaciones_procesadas * 100.0 << "%\n";
    archivo_log_ << "\nESTADÍSTICAS DE COSTOS:\n";
    archivo_log_ << "  Costo mínimo: " << stats_.costo_minimo_global << "\n";
    archivo_log_ << "  Costo máximo: " << stats_.costo_maximo_global << "\n";
    archivo_log_ << "  Costo promedio: " << stats_.costo_promedio << "\n";
    archivo_log_ << "  Rango: " << (stats_.costo_maximo_global - stats_.costo_minimo_global) << "\n";
    archivo_log_.flush();
    
    archivo_resultados_.close();
    archivo_log_.close();
}
