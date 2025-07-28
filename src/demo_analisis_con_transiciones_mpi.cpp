#include "../include/calculador_costos.hpp"
#include "../include/escenario.hpp"
#include <iostream>
#include <fstream>
#include <bitset>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mpi.h>
#include <vector>
#include <algorithm>
#include <limits>

// Función para verificar si un estado está "prendido" (ON)
bool estaPrendido(EstadoMaquina estado) {
    return estado == EstadoMaquina::ON_CALIENTE || 
           estado == EstadoMaquina::ON_TIBIO || 
           estado == EstadoMaquina::ON_FRIO;
}

// Función para generar la cadena de transiciones prender/apagar
std::string generarTransiciones(const std::vector<EstadoMaquina>& estados) {
    std::vector<int> transiciones;
    bool estado_anterior_prendido = false;
    
    for (int hora = 0; hora < 24; hora++) {
        bool estado_actual_prendido = estaPrendido(estados[hora]);
        
        if (hora == 0) {
            // Primera hora: si está prendido, marcar hora 0
            if (estado_actual_prendido) {
                transiciones.push_back(0);
            }
        } else {
            // Detectar transiciones
            if (!estado_anterior_prendido && estado_actual_prendido) {
                // Transición OFF -> ON (prender)
                transiciones.push_back(hora);
            } else if (estado_anterior_prendido && !estado_actual_prendido) {
                // Transición ON -> OFF (apagar)
                transiciones.push_back(hora);
            }
        }
        
        estado_anterior_prendido = estado_actual_prendido;
    }
    
    // Si termina prendido, agregar hora 23
    if (estado_anterior_prendido && !transiciones.empty()) {
        // Solo agregar 23 si no es la última transición ya registrada
        if (transiciones.back() != 23) {
            transiciones.push_back(23);
        }
    }
    
    // Convertir a string
    if (transiciones.empty()) {
        return "";  // Nunca se prendió
    }
    
    std::stringstream ss;
    for (size_t i = 0; i < transiciones.size(); i++) {
        if (i > 0) ss << "-";
        ss << transiciones[i];
    }
    
    return ss.str();
}

// Estructura para almacenar resultados de cada combinación
struct ResultadoCombinacion {
    uint32_t combinacion_id;
    double costo_total;
    bool es_valida;
    int horas_criticas;
    std::string patron_eolica;
    std::string transiciones;
};

int main(int argc, char* argv[]) {
    // Inicializar MPI
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    uint32_t num_combinaciones = 0;
    
    // Solo el proceso 0 lee la entrada
    if (rank == 0) {
        std::cout << "=== ANALIZADOR MASIVO CON TRANSICIONES (MPI) ===\n";
        std::cout << "Procesos MPI: " << size << "\n";
        std::cin >> num_combinaciones;
        
        if (num_combinaciones > 16777216) {
            num_combinaciones = 16777216;
        }
        std::cout << "Procesando " << num_combinaciones << " combinaciones con " << size << " procesos...\n";
    }
    
    // Broadcast del número de combinaciones a todos los procesos
    MPI_Bcast(&num_combinaciones, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
    
    // Calcular el rango de combinaciones para cada proceso
    uint32_t combinaciones_por_proceso = num_combinaciones / size;
    uint32_t resto = num_combinaciones % size;
    
    uint32_t inicio_local = rank * combinaciones_por_proceso + std::min((uint32_t)rank, resto);
    uint32_t fin_local = inicio_local + combinaciones_por_proceso + (rank < resto ? 1 : 0);
    uint32_t num_locales = fin_local - inicio_local;
    
    // Demanda fija según especificación del usuario
    std::vector<double> demanda_fija = {
        300, 200, 100, 100, 100, 200, 300, 500, 800, 1000, 1000, 1000,
        1000, 900, 800, 800, 800, 1000, 1000, 1000, 600, 600, 400, 300
    };
    
    auto inicio = std::chrono::steady_clock::now();
    
    // Variables locales para estadísticas
    double mejor_costo_local = std::numeric_limits<double>::infinity();
    uint32_t combinacion_optima_local = 0;
    uint32_t soluciones_validas_local = 0;
    double suma_costos_local = 0.0;

    // Procesar las combinaciones asignadas a este proceso
    const uint32_t INTERVALO_REPORTE = std::max(100U, num_locales / 10);
    
    for (uint32_t i = 0; i < num_locales; i++) {
        uint32_t combinacion = inicio_local + i;
        
        // Convertir número a patrón binario
        std::bitset<24> patron_eolica(combinacion);
        
        // Configurar escenario (SIN salida de debug)
        Escenario escenario;
        for (int hora = 0; hora < 24; hora++) {
            escenario.setDemanda(hora, demanda_fija[hora]);
            double energia_eolica = patron_eolica[23-hora] ? 500.0 : 0.0;
            escenario.setEnergiaOtrasFuentes(hora, energia_eolica);
        }
        
        // Resolver (silenciar salida)
        CalculadorCostos calculador(escenario);
        calculador.configurarCostos(1.0, 2.5, 5.0);
        
        // Capturar stdout temporalmente
        std::streambuf* old_cout = std::cout.rdbuf();
        std::cout.rdbuf(nullptr);
        Solucion solucion = calculador.resolver();
        std::cout.rdbuf(old_cout);
        
        // Actualizar estadísticas locales
        if (solucion.es_valida) {
            soluciones_validas_local++;
            suma_costos_local += solucion.costo_total;
            if (solucion.costo_total < mejor_costo_local) {
                mejor_costo_local = solucion.costo_total;
                combinacion_optima_local = combinacion;
            }
        }
        
        // Mostrar progreso solo en proceso 0
        if (rank == 0 && ((i + 1) % INTERVALO_REPORTE == 0 || i + 1 == num_locales)) {
            auto ahora = std::chrono::steady_clock::now();
            auto duracion = std::chrono::duration_cast<std::chrono::seconds>(ahora - inicio);
            
            double porcentaje = (double)(i + 1) / num_locales * 100.0;
            double tasa = (double)(i + 1) / std::max(1, (int)duracion.count());
            
            std::cout << "\rProceso 0 - Progreso: " 
                      << std::fixed << std::setprecision(1) << porcentaje << "% | "
                      << "Casos: " << (i + 1) << "/" << num_locales << " | "
                      << "Tasa: " << std::fixed << std::setprecision(0) << tasa << " c/s"
                      << std::flush;
        }
    }
    
    // Sincronizar todos los procesos
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Recopilar estadísticas globales
    uint32_t soluciones_validas_global;
    double suma_costos_global;
    double mejor_costo_global;
    uint32_t combinacion_optima_global;
    
    MPI_Reduce(&soluciones_validas_local, &soluciones_validas_global, 1, MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&suma_costos_local, &suma_costos_global, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&mejor_costo_local, &mejor_costo_global, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    
    // Para encontrar la combinación óptima global, necesitamos un enfoque más elaborado
    struct {
        double costo;
        int rank;
    } local_min, global_min;
    
    local_min.costo = mejor_costo_local;
    local_min.rank = rank;
    
    MPI_Reduce(&local_min, &global_min, 1, MPI_DOUBLE_INT, MPI_MINLOC, 0, MPI_COMM_WORLD);
    
    // El proceso que tiene el mínimo envía su combinación óptima
    if (rank == global_min.rank) {
        MPI_Send(&combinacion_optima_local, 1, MPI_UINT32_T, 0, 0, MPI_COMM_WORLD);
    }
    
    if (rank == 0 && global_min.rank != 0) {
        MPI_Recv(&combinacion_optima_global, 1, MPI_UINT32_T, global_min.rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else if (rank == 0) {
        combinacion_optima_global = combinacion_optima_local;
    }
    
    // Solo el proceso 0 muestra los resultados finales
    if (rank == 0) {
        auto fin = std::chrono::steady_clock::now();
        auto tiempo_total = std::chrono::duration_cast<std::chrono::seconds>(fin - inicio);
        
        std::cout << "\n\n=== PROCESAMIENTO COMPLETADO ===\n";
        std::cout << "Combinaciones procesadas: " << num_combinaciones << "\n";
        std::cout << "Procesos MPI utilizados: " << size << "\n";
        std::cout << "Tiempo total: " << tiempo_total.count() << " segundos\n";
        std::cout << "Tasa promedio: " << std::fixed << std::setprecision(1) 
                  << (double)num_combinaciones / tiempo_total.count() << " combinaciones/segundo\n";
        std::cout << "Soluciones válidas: " << soluciones_validas_global 
                  << " (" << std::fixed << std::setprecision(1) 
                  << (double)soluciones_validas_global / num_combinaciones * 100.0 << "%)\n";
        std::cout << "Mejor costo encontrado: " << mejor_costo_global 
                  << " (combinación #" << combinacion_optima_global << ")\n";
        
        if (soluciones_validas_global > 0) {
            double costo_promedio = suma_costos_global / soluciones_validas_global;
            std::cout << "Costo promedio: " << std::fixed << std::setprecision(2) << costo_promedio << "\n";
        }
    }
    
    MPI_Finalize();
    return 0;
}
        combinacion_optima_global = combinacion_optima_local;
    }
    
    // Solo el proceso 0 escribe los resultados
    if (rank == 0) {
        auto fin = std::chrono::steady_clock::now();
        auto tiempo_total = std::chrono::duration_cast<std::chrono::seconds>(fin - inicio);
        
        std::cout << "\n\n=== RECOPILANDO RESULTADOS DE TODOS LOS PROCESOS ===\n";
        
        // Abrir archivo de resultados
        std::ofstream archivo_resultados("resultados_demo.csv");
        archivo_resultados << "CombinacionID,PatronEolica,CostoTotal,SolucionValida,HorasCriticas,Transiciones\n";
        
        // Escribir resultados del proceso 0
        for (const auto& resultado : resultados_locales) {
            archivo_resultados << resultado.combinacion_id << "," 
                              << resultado.patron_eolica << ","
                              << std::fixed << std::setprecision(2) << resultado.costo_total << ","
                              << (resultado.es_valida ? "SI" : "NO") << ","
                              << resultado.horas_criticas << ","
                              << resultado.transiciones << "\n";
        }
        
        // Recibir y escribir resultados de otros procesos
        for (int p = 1; p < size; p++) {
            uint32_t num_resultados_proceso;
            MPI_Recv(&num_resultados_proceso, 1, MPI_UINT32_T, p, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            for (uint32_t i = 0; i < num_resultados_proceso; i++) {
                uint32_t combinacion_id;
                double costo_total;
                int es_valida_int;
                int horas_criticas;
                char patron_eolica[25];
                char transiciones[100];
                
                MPI_Recv(&combinacion_id, 1, MPI_UINT32_T, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&costo_total, 1, MPI_DOUBLE, p, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&es_valida_int, 1, MPI_INT, p, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&horas_criticas, 1, MPI_INT, p, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(patron_eolica, 25, MPI_CHAR, p, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(transiciones, 100, MPI_CHAR, p, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                archivo_resultados << combinacion_id << "," 
                                  << patron_eolica << ","
                                  << std::fixed << std::setprecision(2) << costo_total << ","
                                  << (es_valida_int ? "SI" : "NO") << ","
                                  << horas_criticas << ","
                                  << transiciones << "\n";
            }
        }
        
        archivo_resultados.close();
        
        std::cout << "\n=== PROCESAMIENTO COMPLETADO ===\n";
        std::cout << "Combinaciones procesadas: " << num_combinaciones << "\n";
        std::cout << "Procesos MPI utilizados: " << size << "\n";
        std::cout << "Tiempo total: " << tiempo_total.count() << " segundos\n";
        std::cout << "Tasa promedio: " << std::fixed << std::setprecision(1) 
                  << (double)num_combinaciones / tiempo_total.count() << " combinaciones/segundo\n";
        std::cout << "Soluciones válidas: " << soluciones_validas_global 
                  << " (" << std::fixed << std::setprecision(1) 
                  << (double)soluciones_validas_global / num_combinaciones * 100.0 << "%)\n";
        std::cout << "Mejor costo encontrado: " << mejor_costo_global 
                  << " (combinación #" << combinacion_optima_global << ")\n";
        
        if (soluciones_validas_global > 0) {
            double costo_promedio = suma_costos_global / soluciones_validas_global;
            std::cout << "Costo promedio: " << std::fixed << std::setprecision(2) << costo_promedio << "\n";
        }
        
        std::cout << "Resultados con transiciones guardados en: resultados_demo.csv\n";
        
    } else {
        // Otros procesos envían sus resultados al proceso 0
        uint32_t num_resultados = resultados_locales.size();
        MPI_Send(&num_resultados, 1, MPI_UINT32_T, 0, 1, MPI_COMM_WORLD);
        
        for (const auto& resultado : resultados_locales) {
            MPI_Send(&resultado.combinacion_id, 1, MPI_UINT32_T, 0, 2, MPI_COMM_WORLD);
            MPI_Send(&resultado.costo_total, 1, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD);
            int es_valida_int = resultado.es_valida ? 1 : 0;
            MPI_Send(&es_valida_int, 1, MPI_INT, 0, 4, MPI_COMM_WORLD);
            MPI_Send(&resultado.horas_criticas, 1, MPI_INT, 0, 5, MPI_COMM_WORLD);
            MPI_Send(resultado.patron_eolica.c_str(), 25, MPI_CHAR, 0, 6, MPI_COMM_WORLD);
            MPI_Send(resultado.transiciones.c_str(), 100, MPI_CHAR, 0, 7, MPI_COMM_WORLD);
        }
    }
    
    MPI_Finalize();
    return 0;
}
