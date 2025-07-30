#include <mpi.h>
#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <sstream>
#include <iomanip>
#include "../include/analisis_exhaustivo_mpi.hpp"

void mostrarAyuda() {
    std::cout << "=== ANÁLISIS EXHAUSTIVO DE ESCENARIOS CON MPI ===" << std::endl;
    std::cout << "Uso: mpirun -np <procesos> ./analisis_exhaustivo_mpi [opciones]" << std::endl;
    std::cout << "\nOpciones:" << std::endl;
    std::cout << "  -l <longitud>  Longitud de las combinaciones (por defecto: modo interactivo)" << std::endl;
    std::cout << "  -o <archivo>   Archivo de salida CSV (se guarda en resultados/)" << std::endl;
    std::cout << "  -h, --help     Mostrar esta ayuda" << std::endl;
    std::cout << "\nEjemplos de uso:" << std::endl;
    std::cout << "  mpirun -np 4 ./analisis_exhaustivo_mpi -l 6" << std::endl;
    std::cout << "  mpirun -np 2 ./analisis_exhaustivo_mpi (modo interactivo)" << std::endl;
    std::cout << "\nFormato de salida CSV:" << std::endl;
    std::cout << "  Combinacion,Encendidos,Costo_Total,Valida" << std::endl;
    std::cout << "  0 1 0 1,0C - 3C,15.0,Si" << std::endl;
    std::cout << "  0 1 1 0,0C - 1C - 2T,12.5,Si" << std::endl;
    std::cout << "  1 1 0 0,1T,12.5,Si" << std::endl;
    std::cout << "\nCódigos de acciones:" << std::endl;
    std::cout << "  F = Encendido en frío / Apagado manteniendo frío" << std::endl;
    std::cout << "  T = Encendido en tibio / Apagado manteniendo tibio" << std::endl;
    std::cout << "  C = Encendido en caliente / Apagado manteniendo caliente" << std::endl;
    std::cout << "\nNota: Los resultados se guardan automáticamente en la carpeta 'resultados/'" << std::endl;
}

int solicitarLongitudInteractiva(int rank) {
    if (rank != 0) return 0; // Solo el proceso 0 solicita entrada
    
    int longitud;
    std::cout << "\n=== SELECCIÓN DE LONGITUD DE ANÁLISIS ===" << std::endl;
    std::cout << "Longitudes recomendadas:" << std::endl;
    std::cout << "  3-5 bits:  Rápido (8-32 combinaciones)" << std::endl;
    std::cout << "  6-8 bits:  Moderado (64-256 combinaciones)" << std::endl;
    std::cout << "  9-12 bits: Lento (512-4096 combinaciones)" << std::endl;
    std::cout << "  13+ bits:  Muy lento (8192+ combinaciones)" << std::endl;
    
    do {
        std::cout << "\nIngrese la longitud de la tira binaria (3-20): ";
        std::cin >> longitud;
        
        if (longitud < 3 || longitud > 20) {
            std::cout << "❌ Por favor ingrese un valor entre 3 y 20." << std::endl;
        } else if (longitud > 15) {
            std::cout << "⚠️  ADVERTENCIA: " << (1 << longitud) << " combinaciones pueden tomar mucho tiempo." << std::endl;
            std::cout << "¿Está seguro? (s/n): ";
            char confirmacion;
            std::cin >> confirmacion;
            if (confirmacion != 's' && confirmacion != 'S') {
                longitud = 0; // Reintentar
            }
        }
    } while (longitud < 3 || longitud > 20);
    
    return longitud;
}

int main(int argc, char** argv) {
    // Inicializar MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Valores por defecto
    int longitud = -1; // Indica modo interactivo
    std::string archivoSalida = "";
    
    // Procesar argumentos de línea de comandos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            if (rank == 0) {
                mostrarAyuda();
            }
            MPI_Finalize();
            return 0;
        }
        else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            longitud = std::stoi(argv[i + 1]);
            i++; // Saltar el siguiente argumento
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            archivoSalida = argv[i + 1];
            i++; // Saltar el siguiente argumento
        }
    }
    
    // Modo interactivo para seleccionar longitud
    if (longitud == -1) {
        longitud = solicitarLongitudInteractiva(rank);
        // Broadcast la longitud a todos los procesos
        MPI_Bcast(&longitud, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    
    // Generar nombre de archivo automático si no se especificó
    if (archivoSalida.empty()) {
        if (rank == 0) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);
            
            std::stringstream ss;
            ss << "analisis_exhaustivo_" << longitud << "bits_" 
               << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".csv";
            archivoSalida = ss.str();
        }
        // Broadcast el nombre de archivo a todos los procesos
        if (rank == 0) {
            int len = archivoSalida.length();
            MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(const_cast<char*>(archivoSalida.c_str()), len, MPI_CHAR, 0, MPI_COMM_WORLD);
        } else {
            int len;
            MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
            char* buffer = new char[len + 1];
            MPI_Bcast(buffer, len, MPI_CHAR, 0, MPI_COMM_WORLD);
            buffer[len] = '\0';
            archivoSalida = std::string(buffer);
            delete[] buffer;
        }
    }
    
    try {
        // Validaciones
        if (longitud < 3 || longitud > 20) {
            if (rank == 0) {
                std::cerr << "❌ Error: La longitud debe estar entre 3 y 20" << std::endl;
                std::cerr << "Longitud " << longitud << " generaría " << (1 << longitud) << " combinaciones" << std::endl;
            }
            MPI_Finalize();
            return 1;
        }
        
        if (rank == 0) {
            std::cout << "\n=== CONFIGURACIÓN DEL ANÁLISIS ===" << std::endl;
            std::cout << "🔢 Longitud de combinaciones: " << longitud << " bits" << std::endl;
            std::cout << "🔄 Total de combinaciones: " << (1 << longitud) << std::endl;
            std::cout << "⚙️  Procesos MPI utilizados: " << size << std::endl;
            std::cout << "📁 Archivo de salida: resultados/" << archivoSalida << std::endl;
            
            // Estimación de tiempo
            int combinaciones = (1 << longitud);
            if (combinaciones <= 256) {
                std::cout << "⏱️  Tiempo estimado: < 1 segundo" << std::endl;
            } else if (combinaciones <= 4096) {
                std::cout << "⏱️  Tiempo estimado: 1-10 segundos" << std::endl;
            } else if (combinaciones <= 65536) {
                std::cout << "⏱️  Tiempo estimado: 10-60 segundos" << std::endl;
            } else {
                std::cout << "⚠️  Tiempo estimado: > 1 minuto" << std::endl;
            }
            
            std::cout << "==========================================" << std::endl;
            std::cout << "🚀 Iniciando análisis..." << std::endl;
        }
        
        // Crear analizador y ejecutar
        AnalisisExhaustivoMPI analizador(rank, size);
        analizador.ejecutarAnalisisExhaustivo(longitud, archivoSalida);
        
        if (rank == 0) {
            std::cout << "=== ANÁLISIS COMPLETADO EXITOSAMENTE ===" << std::endl;
            std::cout << "Consulte el archivo: " << archivoSalida << std::endl;
        }
        
        // CRÍTICO: Limpiar comunicaciones MPI pendientes antes de finalizar
        analizador.limpiarComunicacionesPendientes();
        
    } catch (const std::exception& e) {
        if (rank == 0) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        // Sincronizar antes de finalizar, incluso en caso de error
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Finalize();
        return 1;
    }
    
    // CRÍTICO: Sincronizar todos los procesos antes de MPI_Finalize()
    // Esto previene errores de finalización prematura
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
} 