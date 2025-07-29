#include <mpi.h>
#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>
#include "../include/analisis_exhaustivo_mpi.hpp"

void mostrarAyudaBenchmark() {
    std::cout << "=== BENCHMARK DE RENDIMIENTO - ANÁLISIS EXHAUSTIVO MPI ===" << std::endl;
    std::cout << "Uso: mpirun -np <procesos> ./benchmark_mpi [opciones]" << std::endl;
    std::cout << "\nOpciones:" << std::endl;
    std::cout << "  -b <bits>      Número de bits para análisis (SIN LÍMITE)" << std::endl;
    std::cout << "  -c <nucleos>   Núcleos de CPU disponibles (por defecto: detectar automáticamente)" << std::endl;
    std::cout << "  -v             Modo verbose (mostrar progreso detallado)" << std::endl;
    std::cout << "  -s             Guardar resultados además de métricas" << std::endl;
    std::cout << "  -o <archivo>   Archivo de métricas CSV (por defecto: automático)" << std::endl;
    std::cout << "  -h, --help     Mostrar esta ayuda" << std::endl;
    std::cout << "\nEjemplos de uso:" << std::endl;
    std::cout << "  # Benchmark con 15 bits usando 8 núcleos" << std::endl;
    std::cout << "  mpirun -np 8 ./benchmark_mpi -b 15 -c 8" << std::endl;
    std::cout << "  " << std::endl;
    std::cout << "  # Benchmark grande con 20 bits" << std::endl;
    std::cout << "  mpirun -np 16 ./benchmark_mpi -b 20 -c 16 -v" << std::endl;
    std::cout << "  " << std::endl;
    std::cout << "  # Modo interactivo para configurar parámetros" << std::endl;
    std::cout << "  mpirun -np 4 ./benchmark_mpi" << std::endl;
    std::cout << "\nSalida:" << std::endl;
    std::cout << "  - Archivo CSV con métricas de rendimiento por proceso" << std::endl;
    std::cout << "  - Métricas globales: speedup, eficiencia, balance de carga" << std::endl;
    std::cout << "  - Estadísticas de comunicación MPI y uso de caché" << std::endl;
    std::cout << "\nRango recomendado por número de procesos:" << std::endl;
    std::cout << "  1-2 procesos:   10-20 bits (1K-1M combinaciones)" << std::endl;
    std::cout << "  4-8 procesos:   15-25 bits (32K-33M combinaciones)" << std::endl;
    std::cout << "  16+ procesos:   20-30 bits (1M-1B combinaciones)" << std::endl;
}

ConfiguracionBenchmark solicitarConfiguracion(int rank, int size) {
    ConfiguracionBenchmark config;
    
    if (rank != 0) {
        // Solo el proceso 0 configura, otros esperan broadcast
        return config;
    }
    
    std::cout << "\n=== CONFIGURACIÓN DEL BENCHMARK ===" << std::endl;
    
    // Detectar núcleos automáticamente
    unsigned int nucleosDetectados = std::thread::hardware_concurrency();
    std::cout << "💻 Núcleos detectados automáticamente: " << nucleosDetectados << std::endl;
    
    // Configurar bits
    std::cout << "\n📊 Tamaños de análisis recomendados:" << std::endl;
    std::cout << "  10-15 bits:  1K-32K combinaciones (pruebas rápidas)" << std::endl;
    std::cout << "  16-20 bits:  64K-1M combinaciones (benchmark medio)" << std::endl;
    std::cout << "  21-25 bits:  2M-33M combinaciones (benchmark intensivo)" << std::endl;
    std::cout << "  26+ bits:    67M+ combinaciones (solo clusters/servidores)" << std::endl;
    
    do {
        std::cout << "\nIngrese número de bits para el análisis (5-50): ";
        std::cin >> config.bits;
        
        if (config.bits < 5) {
            std::cout << "❌ Mínimo 5 bits para benchmark útil." << std::endl;
        } else if (config.bits > 35) {
            long long combinaciones = 1LL << config.bits;
            std::cout << "⚠️  ADVERTENCIA: " << config.bits << " bits = " << combinaciones 
                      << " combinaciones." << std::endl;
            std::cout << "Esto puede tomar MUCHO tiempo. ¿Continuar? (s/n): ";
            char confirmacion;
            std::cin >> confirmacion;
            if (confirmacion != 's' && confirmacion != 'S') {
                config.bits = 0; // Reintentar
            }
        }
    } while (config.bits < 5);
    
    // Configurar núcleos
    std::cout << "\nProcesos MPI disponibles: " << size << std::endl;
    do {
        std::cout << "Núcleos de CPU a utilizar (1-" << nucleosDetectados << ", 0=automático): ";
        std::cin >> config.nucleos;
        
        if (config.nucleos == 0) {
            config.nucleos = nucleosDetectados;
            std::cout << "✅ Usando " << config.nucleos << " núcleos automáticamente." << std::endl;
        } else if (config.nucleos > (int)nucleosDetectados) {
            std::cout << "⚠️  Advertencia: Especificaste más núcleos que los disponibles." << std::endl;
        }
    } while (config.nucleos < 1);
    
    config.procesos = size;
    
    // Configurar opciones adicionales
    std::cout << "\n¿Modo verbose (mostrar progreso detallado)? (s/n): ";
    char verbose;
    std::cin >> verbose;
    config.modoVerboso = (verbose == 's' || verbose == 'S');
    
    std::cout << "¿Guardar resultados detallados además de métricas? (s/n): ";
    char guardar;
    std::cin >> guardar;
    config.guardarResultados = (guardar == 's' || guardar == 'S');
    
    // Generar nombre de archivo automático
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << "benchmark_" << config.bits << "bits_" << config.procesos << "proc_" 
       << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".csv";
    config.archivoMetricas = ss.str();
    
    return config;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (rank == 0) {
        std::cout << "🚀 INICIANDO BENCHMARK DE RENDIMIENTO HPC" << std::endl;
        std::cout << "Procesos MPI: " << size << std::endl;
    }
    
    ConfiguracionBenchmark config;
    
    // Valores por defecto
    config.bits = -1; // Indica modo interactivo
    config.nucleos = 0; // Detectar automáticamente
    config.modoVerboso = false;
    config.guardarResultados = false;
    config.archivoMetricas = "";
    config.procesos = size;
    
    // Procesar argumentos de línea de comandos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            if (rank == 0) {
                mostrarAyudaBenchmark();
            }
            MPI_Finalize();
            return 0;
        }
        else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            config.bits = std::stoi(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            config.nucleos = std::stoi(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "-v") == 0) {
            config.modoVerboso = true;
        }
        else if (strcmp(argv[i], "-s") == 0) {
            config.guardarResultados = true;
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            config.archivoMetricas = argv[i + 1];
            i++;
        }
    }
    
    // Modo interactivo si no se especificaron parámetros críticos
    if (config.bits == -1) {
        config = solicitarConfiguracion(rank, size);
        
        // Broadcast la configuración a todos los procesos
        MPI_Bcast(&config.bits, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&config.nucleos, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&config.modoVerboso, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);
        MPI_Bcast(&config.guardarResultados, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);
        
        // Broadcast del nombre de archivo
        if (rank == 0) {
            int len = config.archivoMetricas.length();
            MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(const_cast<char*>(config.archivoMetricas.c_str()), len, MPI_CHAR, 0, MPI_COMM_WORLD);
        } else {
            int len;
            MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
            char* buffer = new char[len + 1];
            MPI_Bcast(buffer, len, MPI_CHAR, 0, MPI_COMM_WORLD);
            buffer[len] = '\0';
            config.archivoMetricas = std::string(buffer);
            delete[] buffer;
        }
    }
    
    // Configurar núcleos automáticamente si es necesario
    if (config.nucleos == 0) {
        config.nucleos = std::thread::hardware_concurrency();
    }
    
    // Generar nombre de archivo automático si no se especificó
    if (config.archivoMetricas.empty() && rank == 0) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        std::stringstream ss;
        ss << "benchmark_" << config.bits << "bits_" << config.procesos << "proc_" 
           << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".csv";
        config.archivoMetricas = ss.str();
    }
    
    try {
        // Validaciones
        if (config.bits < 5 || config.bits > 50) {
            if (rank == 0) {
                std::cerr << "❌ Error: Bits debe estar entre 5 y 50" << std::endl;
                std::cerr << "Para benchmarks útiles, recomendamos 10-30 bits" << std::endl;
            }
            MPI_Finalize();
            return 1;
        }
        
        if (rank == 0) {
            long long totalCombinaciones = 1LL << config.bits;
            std::cout << "\n=== CONFIGURACIÓN FINAL DEL BENCHMARK ===" << std::endl;
            std::cout << "🔢 Bits: " << config.bits << std::endl;
            std::cout << "🔄 Total de combinaciones: " << totalCombinaciones << std::endl;
            std::cout << "💻 Núcleos configurados: " << config.nucleos << std::endl;
            std::cout << "⚙️  Procesos MPI: " << config.procesos << std::endl;
            std::cout << "📊 Combinaciones por proceso: " << (totalCombinaciones / config.procesos) << std::endl;
            std::cout << "🗣️  Modo verbose: " << (config.modoVerboso ? "SÍ" : "NO") << std::endl;
            std::cout << "💾 Guardar resultados: " << (config.guardarResultados ? "SÍ" : "NO") << std::endl;
            std::cout << "📁 Archivo de métricas: resultados/" << config.archivoMetricas << std::endl;
            
            // Estimación de tiempo
            double estimacionSegundos = totalCombinaciones / (500000.0 * config.procesos); // ~500K comb/seg/proceso
            if (estimacionSegundos < 1) {
                std::cout << "⏱️  Tiempo estimado: < 1 segundo" << std::endl;
            } else if (estimacionSegundos < 60) {
                std::cout << "⏱️  Tiempo estimado: " << (int)estimacionSegundos << " segundos" << std::endl;
            } else if (estimacionSegundos < 3600) {
                std::cout << "⏱️  Tiempo estimado: " << (int)(estimacionSegundos/60) << " minutos" << std::endl;
            } else {
                std::cout << "⏱️  Tiempo estimado: " << (int)(estimacionSegundos/3600) << " horas" << std::endl;
            }
            
            std::cout << "===========================================" << std::endl;
            std::cout << "🚀 INICIANDO BENCHMARK..." << std::endl;
        }
        
        // Crear y ejecutar el benchmark
        AnalisisExhaustivoMPI benchmark(rank, size);
        benchmark.ejecutarBenchmark(config);
        
        if (rank == 0) {
            std::cout << "=== BENCHMARK COMPLETADO EXITOSAMENTE ===" << std::endl;
            std::cout << "Consulte el archivo de métricas: resultados/" << config.archivoMetricas << std::endl;
        }
        
    } catch (const std::exception& e) {
        if (rank == 0) {
            std::cerr << "❌ Error durante el benchmark: " << e.what() << std::endl;
        }
        MPI_Finalize();
        return 1;
    }
    
    MPI_Finalize();
    return 0;
} 