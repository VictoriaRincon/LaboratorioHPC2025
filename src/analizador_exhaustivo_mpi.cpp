#include "analizador_exhaustivo_mpi.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>

// Constructor
AnalizadorExhaustivoMPI::AnalizadorExhaustivoMPI() {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &size_);
    
    // Configuración por defecto
    intervalo_reporte_ = 1000;
    guardar_todas_soluciones_ = false;
    umbral_costo_ = 1000.0;
    
    // Archivos por defecto con rank incluido
    std::stringstream ss_res, ss_log;
    ss_res << "resultados_mpi_rank" << rank_ << ".csv";
    ss_log << "log_mpi_rank" << rank_ << ".txt";
    archivo_resultados_ = ss_res.str();
    archivo_log_ = ss_log.str();
}

// Destructor
AnalizadorExhaustivoMPI::~AnalizadorExhaustivoMPI() {
    // MPI_Finalize se llama externamente
}

void AnalizadorExhaustivoMPI::configurarDemanda(const std::vector<double>& demanda) {
    demanda_fija_ = demanda;
}

void AnalizadorExhaustivoMPI::configurarArchivos(const std::string& archivo_resultados, const std::string& archivo_log) {
    // Agregar rank al nombre del archivo para evitar conflictos
    std::stringstream ss_res, ss_log;
    
    size_t pos_ext = archivo_resultados.find_last_of('.');
    if (pos_ext != std::string::npos) {
        ss_res << archivo_resultados.substr(0, pos_ext) << "_rank" << rank_ << archivo_resultados.substr(pos_ext);
    } else {
        ss_res << archivo_resultados << "_rank" << rank_;
    }
    
    pos_ext = archivo_log.find_last_of('.');
    if (pos_ext != std::string::npos) {
        ss_log << archivo_log.substr(0, pos_ext) << "_rank" << rank_ << archivo_log.substr(pos_ext);
    } else {
        ss_log << archivo_log << "_rank" << rank_;
    }
    
    archivo_resultados_ = ss_res.str();
    archivo_log_ = ss_log.str();
}

void AnalizadorExhaustivoMPI::configurarReporte(uint32_t intervalo, bool guardar_todas, double umbral) {
    intervalo_reporte_ = intervalo;
    guardar_todas_soluciones_ = guardar_todas;
    umbral_costo_ = umbral;
}

void AnalizadorExhaustivoMPI::calcularRangoTrabajo(uint32_t total_combinaciones, uint32_t& desde, uint32_t& hasta) {
    uint32_t combinaciones_por_proceso = total_combinaciones / size_;
    uint32_t resto = total_combinaciones % size_;
    
    desde = rank_ * combinaciones_por_proceso;
    hasta = desde + combinaciones_por_proceso;
    
    // Distribuir el resto entre los primeros procesos
    if ((uint32_t)rank_ < resto) {
        desde += rank_;
        hasta += rank_ + 1;
    } else {
        desde += resto;
        hasta += resto;
    }
}

void AnalizadorExhaustivoMPI::configurarEscenario(Escenario& escenario, const std::bitset<24>& patron_eolica) {
    // Configurar energía eólica según el patrón
    std::vector<double> energia_eolica(24);
    for (int hora = 0; hora < 24; hora++) {
        energia_eolica[hora] = patron_eolica[hora] ? 500.0 : 0.0;
    }
    
    // Usar configurarDirecto para establecer demanda y energía eólica
    escenario.configurarDirecto(demanda_fija_, energia_eolica);
}

void AnalizadorExhaustivoMPI::inicializarArchivos() {
    // Solo crear archivos de encabezado si no existen
    std::ofstream archivo_res(archivo_resultados_, std::ios::app);
    archivo_res.seekp(0, std::ios::end);
    if (archivo_res.tellp() == 0) {
        archivo_res << "CombinacionID,PatronEolica,CostoTotal,SolucionValida,HorasCriticas\n";
    }
    archivo_res.close();
    
    std::ofstream archivo_log(archivo_log_);
    archivo_log << "=== ANÁLISIS EXHAUSTIVO MPI - Proceso " << rank_ << " ===\n";
    archivo_log << "Proceso: " << rank_ << "/" << size_ << "\n";
    archivo_log << "Archivo de resultados: " << archivo_resultados_ << "\n\n";
    archivo_log.close();
}

void AnalizadorExhaustivoMPI::guardarResultado(const ResultadoCombinacionMPI& resultado) {
    if (!guardar_todas_soluciones_ && resultado.costo_total > umbral_costo_) {
        return;
    }
    
    std::ofstream archivo_res(archivo_resultados_, std::ios::app);
    std::bitset<24> patron(resultado.patron_eolica_bits);
    
    archivo_res << resultado.combinacion_id << ","
                << patron.to_string() << ","
                << std::fixed << std::setprecision(2) << resultado.costo_total << ","
                << (resultado.solucion_valida ? "SI" : "NO") << ","
                << resultado.horas_criticas << "\n";
    archivo_res.close();
}

void AnalizadorExhaustivoMPI::reportarProgreso() {
    if (esMaster()) {
        auto ahora = std::chrono::steady_clock::now();
        auto duracion = std::chrono::duration_cast<std::chrono::seconds>(ahora - stats_.tiempo_inicio);
        
        double porcentaje = (double)stats_.combinaciones_procesadas / stats_.combinaciones_totales * 100.0;
        double casos_por_segundo = (double)stats_.combinaciones_procesadas / std::max((long long)duracion.count(), 1LL);
        
        uint32_t total_global = stats_.combinaciones_totales * size_;
        uint32_t procesadas_global = stats_.combinaciones_procesadas * size_;
        
        std::cout << "\rProgreso MPI: " << std::fixed << std::setprecision(1) << porcentaje << "% | "
                  << "Procesos: " << size_ << " | "
                  << "Casos: " << procesadas_global << "/" << total_global << " | "
                  << "Válidos: " << stats_.soluciones_validas << " | "
                  << "Mejor: " << std::setprecision(2) << stats_.costo_minimo_global << " | "
                  << "Tasa: " << std::setprecision(0) << casos_por_segundo << " c/s";
        std::cout.flush();
        
        stats_.ultimo_reporte = ahora;
    }
}

void AnalizadorExhaustivoMPI::ejecutarAnalisisParcialMPI(uint32_t desde_global, uint32_t hasta_global) {
    uint32_t total_combinaciones = hasta_global - desde_global;
    uint32_t desde_local, hasta_local;
    
    // Calcular rango de trabajo para este proceso
    calcularRangoTrabajo(total_combinaciones, desde_local, hasta_local);
    
    // Ajustar al rango global
    desde_local += desde_global;
    hasta_local += desde_global;
    
    if (esMaster()) {
        std::cout << "\n=== ANÁLISIS EXHAUSTIVO MPI ===\n";
        std::cout << "Procesos MPI: " << size_ << "\n";
        std::cout << "Rango global: [" << desde_global << " - " << hasta_global << "]\n";
        std::cout << "Combinaciones por proceso: ~" << (total_combinaciones / size_) << "\n\n";
    }
    
    // Mostrar rango específico de cada proceso
    std::cout << "Proceso " << rank_ << " procesará: [" << desde_local << " - " << hasta_local << "]\n";
    MPI_Barrier(MPI_COMM_WORLD); // Sincronizar salida
    
    // Configurar estadísticas locales
    stats_.combinaciones_totales = hasta_local - desde_local;
    stats_.tiempo_inicio = std::chrono::steady_clock::now();
    stats_.ultimo_reporte = stats_.tiempo_inicio;
    
    inicializarArchivos();
    
    double suma_costos = 0.0;
    
    // Procesar rango asignado a este proceso
    for (uint32_t combinacion = desde_local; combinacion < hasta_local; combinacion++) {
        std::bitset<24> patron_eolica(combinacion);
        
        // Configurar escenario
        Escenario escenario;
        configurarEscenario(escenario, patron_eolica);
        
        // Resolver
        CalculadorCostos calculador(escenario);
        calculador.configurarCostos(1.0, 2.5, 5.0);
        Solucion solucion = calculador.resolver();
        
        // Almacenar resultado
        ResultadoCombinacionMPI resultado;
        resultado.combinacion_id = combinacion;
        resultado.patron_eolica_bits = (uint32_t)patron_eolica.to_ulong();
        resultado.costo_total = solucion.costo_total;
        resultado.solucion_valida = solucion.es_valida ? 1 : 0;
        
        // Calcular horas críticas
        resultado.horas_criticas = 0;
        for (int hora = 0; hora < 24; hora++) {
            if (!escenario.demandaCubiertaConEO(hora)) {
                resultado.horas_criticas++;
            }
        }
        
        // Actualizar estadísticas locales
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
        
        // Reportar progreso (solo master)
        if (stats_.combinaciones_procesadas % intervalo_reporte_ == 0) {
            reportarProgreso();
        }
    }
    
    // Sincronizar todos los procesos al final
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (esMaster()) {
        std::cout << "\n\n=== RECOLECTANDO RESULTADOS MPI ===\n";
    }
    
    // Fusionar estadísticas de todos los procesos
    fusionarEstadisticas();
    
    if (esMaster()) {
        mostrarEstadisticas();
    }
}

void AnalizadorExhaustivoMPI::ejecutarAnalisisCompletoMPI() {
    ejecutarAnalisisParcialMPI(0, 16777216);
}

void AnalizadorExhaustivoMPI::fusionarEstadisticas() {
    // Recolectar estadísticas de todos los procesos
    uint32_t stats_local[3] = {stats_.combinaciones_procesadas, stats_.soluciones_validas, 0};
    double costos_local[3] = {stats_.costo_minimo_global, stats_.costo_maximo_global, stats_.costo_promedio};
    
    uint32_t stats_global[3];
    double costos_global[3];
    
    // Sumar contadores
    MPI_Reduce(stats_local, stats_global, 3, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);
    
    // Obtener mínimo y máximo global
    MPI_Reduce(&costos_local[0], &costos_global[0], 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD); // mínimo
    MPI_Reduce(&costos_local[1], &costos_global[1], 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD); // máximo
    
    if (esMaster()) {
        stats_.combinaciones_procesadas = stats_global[0];
        stats_.soluciones_validas = stats_global[1];
        stats_.costo_minimo_global = costos_global[0];
        stats_.costo_maximo_global = costos_global[1];
        
        // Calcular promedio global (aproximado)
        if (stats_.soluciones_validas > 0) {
            // Esto es una aproximación - en un caso real se necesitaría recolectar todas las sumas
            stats_.costo_promedio = (costos_global[0] + costos_global[1]) / 2.0;
        }
    }
}

void AnalizadorExhaustivoMPI::mostrarEstadisticas() const {
    if (!esMaster()) return;
    
    auto tiempo_fin = std::chrono::steady_clock::now();
    auto duracion_total = std::chrono::duration_cast<std::chrono::seconds>(tiempo_fin - stats_.tiempo_inicio);
    
    std::cout << "\n=== ESTADÍSTICAS FINALES MPI ===\n";
    std::cout << "Procesos utilizados: " << size_ << "\n";
    std::cout << "Combinaciones procesadas: " << stats_.combinaciones_procesadas << "\n";
    std::cout << "Soluciones válidas: " << stats_.soluciones_validas << "\n";
    std::cout << "Tiempo total: " << duracion_total.count() << " segundos\n";
    std::cout << "Casos por segundo (global): " << (double)stats_.combinaciones_procesadas / std::max((long long)duracion_total.count(), 1LL) << "\n";
    
    if (stats_.soluciones_validas > 0) {
        std::cout << "Costo mínimo encontrado: " << std::fixed << std::setprecision(2) << stats_.costo_minimo_global << "\n";
        std::cout << "Costo máximo encontrado: " << stats_.costo_maximo_global << "\n";
    }
    
    std::cout << "\nArchivos generados:\n";
    for (int i = 0; i < size_; i++) {
        std::cout << "  - resultados_rank" << i << ".csv\n";
        std::cout << "  - log_rank" << i << ".txt\n";
    }
    std::cout << "\n";
}

void AnalizadorExhaustivoMPI::mostrarConfiguracion() const {
    if (esMaster()) {
        std::cout << "Configuración MPI:\n";
        std::cout << "- Procesos: " << size_ << "\n";
        std::cout << "- Intervalo de reporte: " << intervalo_reporte_ << "\n";
        std::cout << "- Guardar todas las soluciones: " << (guardar_todas_soluciones_ ? "SÍ" : "NO") << "\n";
        std::cout << "- Umbral de costo: " << umbral_costo_ << "\n\n";
    }
}

// Funciones auxiliares MPI
void inicializarMPI(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
}

void finalizarMPI() {
    MPI_Finalize();
} 