#include "analizador_exhaustivo_mpi.hpp"
#include <iostream>
#include <vector>
#include <string>

void mostrarMenuMPI(int rank) {
    if (rank == 0) {  // Solo el proceso master muestra el menú
        std::cout << "\n=== ANALIZADOR EXHAUSTIVO MPI - MÁQUINA DE ESTADOS ===\n";
        std::cout << "1. Ejecutar análisis completo MPI (16,777,216 combinaciones)\n";
        std::cout << "2. Ejecutar análisis parcial MPI (especificar rango)\n";
        std::cout << "3. Ejecutar prueba pequeña MPI (1,000 combinaciones)\n";
        std::cout << "4. Ejecutar prueba mediana MPI (100,000 combinaciones)\n";
        std::cout << "5. Ejecutar prueba grande MPI (1,000,000 combinaciones)\n";
        std::cout << "6. Configurar parámetros y ejecutar MPI\n";
        std::cout << "0. Salir\n";
        std::cout << "Selecciona una opción: ";
    }
}

int main(int argc, char* argv[]) {
    // Inicializar MPI
    inicializarMPI(argc, argv);
    
    AnalizadorExhaustivoMPI analizador;
    int rank = analizador.getRank();
    int size = analizador.getSize();
    
    if (rank == 0) {
        std::cout << "=== SISTEMA DE ANÁLISIS EXHAUSTIVO MPI ===\n";
        std::cout << "Procesos MPI disponibles: " << size << "\n";
        std::cout << "Este programa distribuirá el análisis entre todos los procesos\n";
        std::cout << "para acelerar significativamente el procesamiento.\n\n";
    }
    
    // Configurar demanda fija (como especificó el usuario)
    std::vector<double> demanda_fija = {
        300, 200, 100, 100, 100, 200, 300, 500, 800, 1000, 1000, 1000,
        1000, 900, 800, 800, 800, 1000, 1000, 1000, 600, 600, 400, 300
    };
    
    analizador.configurarDemanda(demanda_fija);
    
    if (rank == 0) {
        std::cout << "Demanda configurada por hora:\n";
        for (int h = 0; h < 24; h++) {
            std::cout << "H" << h << ":" << demanda_fija[h] << " ";
            if ((h + 1) % 6 == 0) std::cout << "\n";
        }
        std::cout << "\n";
        std::cout << "Energía eólica: 0 o 500 para cada hora (2^24 = 16,777,216 combinaciones)\n\n";
    }
    
    int opcion;
    do {
        mostrarMenuMPI(rank);
        
        if (rank == 0) {
            std::cin >> opcion;
        }
        
        // Broadcast la opción a todos los procesos
        MPI_Bcast(&opcion, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        switch (opcion) {
            case 1: {
                if (rank == 0) {
                    std::cout << "\n⚠️  ADVERTENCIA: El análisis completo con " << size << " procesos\n";
                    std::cout << "aún puede tomar HORAS o DÍAS dependiendo de tu hardware.\n";
                    std::cout << "¿Estás seguro? (s/N): ";
                    char confirmar;
                    std::cin >> confirmar;
                    
                    // Broadcast la confirmación
                    int continuar = (confirmar == 's' || confirmar == 'S') ? 1 : 0;
                    MPI_Bcast(&continuar, 1, MPI_INT, 0, MPI_COMM_WORLD);
                    
                    if (continuar) {
                        analizador.configurarArchivos("resultados_completos_mpi.csv", "log_completo_mpi.txt");
                        analizador.configurarReporte(10000, false, 100.0);
                        analizador.mostrarConfiguracion();
                        analizador.ejecutarAnalisisCompletoMPI();
                    }
                } else {
                    int continuar;
                    MPI_Bcast(&continuar, 1, MPI_INT, 0, MPI_COMM_WORLD);
                    
                    if (continuar) {
                        analizador.configurarArchivos("resultados_completos_mpi.csv", "log_completo_mpi.txt");
                        analizador.configurarReporte(10000, false, 100.0);
                        analizador.ejecutarAnalisisCompletoMPI();
                    }
                }
                break;
            }
            
            case 2: {
                uint32_t desde, hasta;
                if (rank == 0) {
                    std::cout << "Ingresa el rango de combinaciones:\n";
                    std::cout << "Desde (0 - 16777215): ";
                    std::cin >> desde;
                    std::cout << "Hasta (0 - 16777215): ";
                    std::cin >> hasta;
                    
                    if (desde >= hasta || hasta > 16777216) {
                        std::cout << "Rango inválido.\n";
                        desde = hasta = 0;  // Marcar como inválido
                    }
                }
                
                // Broadcast los parámetros
                MPI_Bcast(&desde, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
                MPI_Bcast(&hasta, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
                
                if (desde < hasta) {
                    std::string archivo_res = "resultados_parcial_mpi_" + std::to_string(desde) + "_" + std::to_string(hasta) + ".csv";
                    std::string archivo_log = "log_parcial_mpi_" + std::to_string(desde) + "_" + std::to_string(hasta) + ".txt";
                    
                    analizador.configurarArchivos(archivo_res, archivo_log);
                    analizador.configurarReporte(1000, true);
                    if (rank == 0) analizador.mostrarConfiguracion();
                    analizador.ejecutarAnalisisParcialMPI(desde, hasta);
                }
                break;
            }
            
            case 3: {
                if (rank == 0) {
                    std::cout << "\nEjecutando prueba pequeña MPI (1,000 combinaciones)...\n";
                    std::cout << "Cada proceso procesará ~" << (1000 / size) << " combinaciones.\n";
                }
                analizador.configurarArchivos("resultados_prueba_1k_mpi.csv", "log_prueba_1k_mpi.txt");
                analizador.configurarReporte(100, true);
                if (rank == 0) analizador.mostrarConfiguracion();
                analizador.ejecutarAnalisisParcialMPI(0, 1000);
                break;
            }
            
            case 4: {
                if (rank == 0) {
                    std::cout << "\nEjecutando prueba mediana MPI (100,000 combinaciones)...\n";
                    std::cout << "Con " << size << " procesos, esto debería tomar minutos en lugar de horas.\n";
                    std::cout << "Cada proceso procesará ~" << (100000 / size) << " combinaciones.\n";
                }
                analizador.configurarArchivos("resultados_prueba_100k_mpi.csv", "log_prueba_100k_mpi.txt");
                analizador.configurarReporte(5000, false, 80.0);
                if (rank == 0) analizador.mostrarConfiguracion();
                analizador.ejecutarAnalisisParcialMPI(0, 100000);
                break;
            }
            
            case 5: {
                if (rank == 0) {
                    std::cout << "\nEjecutando prueba grande MPI (1,000,000 combinaciones)...\n";
                    std::cout << "Con " << size << " procesos, esto demostrará el poder de MPI.\n";
                    std::cout << "Cada proceso procesará ~" << (1000000 / size) << " combinaciones.\n";
                }
                analizador.configurarArchivos("resultados_prueba_1M_mpi.csv", "log_prueba_1M_mpi.txt");
                analizador.configurarReporte(10000, false, 70.0);
                if (rank == 0) analizador.mostrarConfiguracion();
                analizador.ejecutarAnalisisParcialMPI(0, 1000000);
                break;
            }
            
            case 6: {
                uint32_t desde, hasta, intervalo;
                double umbral_costo;
                int guardar_todas_int;
                
                if (rank == 0) {
                    std::cout << "\n=== CONFIGURACIÓN PERSONALIZADA MPI ===\n";
                    std::cout << "Desde: ";
                    std::cin >> desde;
                    std::cout << "Hasta: ";
                    std::cin >> hasta;
                    std::cout << "Intervalo de reporte: ";
                    std::cin >> intervalo;
                    std::cout << "Umbral de costo (solo guardar <= X): ";
                    std::cin >> umbral_costo;
                    std::cout << "¿Guardar todas las soluciones? (1=Sí, 0=No): ";
                    std::cin >> guardar_todas_int;
                }
                
                // Broadcast todos los parámetros
                MPI_Bcast(&desde, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
                MPI_Bcast(&hasta, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
                MPI_Bcast(&intervalo, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
                MPI_Bcast(&umbral_costo, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Bcast(&guardar_todas_int, 1, MPI_INT, 0, MPI_COMM_WORLD);
                
                bool guardar_todas = (guardar_todas_int == 1);
                
                std::string archivo_res = "resultados_custom_mpi_" + std::to_string(desde) + "_" + std::to_string(hasta) + ".csv";
                std::string archivo_log = "log_custom_mpi_" + std::to_string(desde) + "_" + std::to_string(hasta) + ".txt";
                
                analizador.configurarArchivos(archivo_res, archivo_log);
                analizador.configurarReporte(intervalo, guardar_todas, umbral_costo);
                if (rank == 0) analizador.mostrarConfiguracion();
                analizador.ejecutarAnalisisParcialMPI(desde, hasta);
                break;
            }
            
            case 0:
                if (rank == 0) {
                    std::cout << "¡Análisis MPI terminado!\n";
                }
                break;
                
            default:
                if (rank == 0) {
                    std::cout << "Opción inválida.\n";
                }
                break;
        }
        
    } while (opcion != 0);
    
    if (rank == 0) {
        std::cout << "\n=== INFORMACIÓN DE ARCHIVOS GENERADOS MPI ===\n";
        std::cout << "Los resultados se han distribuido en " << size << " archivos separados:\n";
        for (int i = 0; i < size; i++) {
            std::cout << "- resultados_*_rank" << i << ".csv\n";
            std::cout << "- log_*_rank" << i << ".txt\n";
        }
        std::cout << "\nCada archivo contiene los resultados procesados por un proceso MPI específico.\n";
        std::cout << "Para análisis posterior, puedes combinar todos los archivos CSV o analizarlos por separado.\n\n";
        
        std::cout << "RENDIMIENTO MPI:\n";
        std::cout << "Al usar " << size << " procesos, el tiempo de procesamiento se reduce\n";
        std::cout << "teóricamente por un factor de " << size << "x comparado con la versión secuencial.\n\n";
        
        std::cout << "¡Gracias por usar el Analizador Exhaustivo MPI!\n";
    }
    
    // Finalizar MPI
    finalizarMPI();
    
    return 0;
} 