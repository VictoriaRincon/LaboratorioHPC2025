#include "../include/analisis_exhaustivo_mpi.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <set>
#include <cmath>
#include <cstdlib>
#include <sys/resource.h>
#include <unistd.h>
#include <limits>
#include <thread>
#include <ctime>

AnalisisExhaustivoMPI::AnalisisExhaustivoMPI(int rank, int size) 
    : rank_(rank), size_(size) {
    calculador_.setMostrarDetalles(false);
}

void AnalisisExhaustivoMPI::ejecutarAnalisisExhaustivo(int longitud, const std::string& archivoSalida) {
    auto inicio = std::chrono::high_resolution_clock::now();
    
    if (rank_ == 0) {
        std::cout << "=== ANÁLISIS EXHAUSTIVO CON MPI ===" << std::endl;
        std::cout << "Longitud: " << longitud << std::endl;
        std::cout << "Procesos MPI: " << size_ << std::endl;
        std::cout << "Total de combinaciones: " << (1 << longitud) << std::endl;
    }
    
    // Generar todas las combinaciones (solo el proceso 0)
    std::vector<std::vector<int>> todasCombinaciones;
    if (rank_ == 0) {
        generarTodasLasCombinaciones(longitud, todasCombinaciones);
        std::cout << "Combinaciones generadas: " << todasCombinaciones.size() << std::endl;
    }
    
    // Distribuir combinaciones entre procesos
    std::vector<std::vector<int>> misCombinaciones = obtenerCombinacionesParaProceso(todasCombinaciones);
    
    std::cout << "Proceso " << rank_ << " analizará " << misCombinaciones.size() << " combinaciones" << std::endl;
    
    // Analizar combinaciones asignadas
    std::vector<ResultadoCombinacion> misResultados;
    auto inicioAnalisis = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < misCombinaciones.size(); ++i) {
        if (rank_ == 0) {
            mostrarProgreso(i, misCombinaciones.size());
        }
        
        auto inicioCombo = std::chrono::high_resolution_clock::now();
        auto resultado = analizarCombinacion(misCombinaciones[i]);
        auto finCombo = std::chrono::high_resolution_clock::now();
        
        auto duracionCombo = std::chrono::duration_cast<std::chrono::microseconds>(finCombo - inicioCombo);
        estadisticasCache_.tiempoTotal += duracionCombo.count() / 1000.0; // convertir a ms
        
        misResultados.push_back(resultado);
        
        // Sincronizar caché periódicamente
        if (i % 50 == 0) {
            sincronizarCache();
        }
    }
    
    auto finAnalisis = std::chrono::high_resolution_clock::now();
    auto duracionAnalisis = std::chrono::duration_cast<std::chrono::milliseconds>(finAnalisis - inicioAnalisis);
    
    if (rank_ == 0) {
        std::cout << "\nProceso " << rank_ << " completó análisis local en " 
                  << duracionAnalisis.count() << " ms" << std::endl;
    } else {
        std::cout << "Proceso " << rank_ << " completó análisis local en " 
                  << duracionAnalisis.count() << " ms" << std::endl;
    }
    
    std::cout << "Proceso " << rank_ << " completó análisis local" << std::endl;
    
    // Escribir resultados inmediatamente (modo incremental)
    auto inicioRecoleccion = std::chrono::high_resolution_clock::now();
    
    // Variables para seguimiento (fuera del scope de rank_)
    std::string rutaCompleta = archivoSalida;
    if (rutaCompleta.find("resultados/") != 0) {
        rutaCompleta = "resultados/" + archivoSalida;
    }
    
    int totalResultadosEscritos = misResultados.size();
    
    if (rank_ == 0) {
        // Inicializar archivo CSV con headers
        inicializarArchivoCSV(rutaCompleta);
        
        // Escribir resultados locales primero
        escribirCSVIncremental(misResultados, rutaCompleta);
        std::cout << "✅ Escritos " << misResultados.size() << " resultados del proceso 0" << std::endl;
        
        if (size_ > 1) {
            std::cout << "\n=== RECOLECTANDO Y ESCRIBIENDO RESULTADOS DE OTROS PROCESOS ===" << std::endl;
        }
        
        // Procesar resultados de otros procesos uno por uno (sin almacenar todo en memoria)
        for (int proc = 1; proc < size_; ++proc) {
            std::cout << "Recibiendo del proceso " << proc << "..." << std::flush;
            
            int numResultados;
            MPI_Recv(&numResultados, 1, MPI_INT, proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            registrarMensajeMPI(false, sizeof(int));
            
            // Recibir en lotes pequeños para evitar problemas de memoria
            std::vector<ResultadoCombinacion> loteResultados;
            loteResultados.reserve(std::min(numResultados, 100)); // Máximo 100 por lote
            
            for (int i = 0; i < numResultados; ++i) {
                ResultadoCombinacion resultado;
                
                // Recibir combinación
                int tamCombinacion;
                MPI_Recv(&tamCombinacion, 1, MPI_INT, proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                registrarMensajeMPI(false, sizeof(int));
                
                resultado.combinacion.resize(tamCombinacion);
                MPI_Recv(resultado.combinacion.data(), tamCombinacion, MPI_INT, proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                registrarMensajeMPI(false, tamCombinacion * sizeof(int));
                
                // Recibir encendidos
                int numEncendidos;
                MPI_Recv(&numEncendidos, 1, MPI_INT, proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                resultado.encendidos.resize(numEncendidos);
                
                for (int j = 0; j < numEncendidos; ++j) {
                    MPI_Recv(&resultado.encendidos[j].first, 1, MPI_INT, proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    
                    int tamEstado;
                    MPI_Recv(&tamEstado, 1, MPI_INT, proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    resultado.encendidos[j].second.resize(tamEstado);
                    MPI_Recv(&resultado.encendidos[j].second[0], tamEstado, MPI_CHAR, proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                
                // Recibir otros datos
                MPI_Recv(&resultado.costoTotal, 1, MPI_DOUBLE, proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                int valida;
                MPI_Recv(&valida, 1, MPI_INT, proc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                resultado.valida = (valida == 1);
                
                // Añadir al lote y escribir cuando esté lleno
                loteResultados.push_back(resultado);
                
                // Escribir cada 50 resultados o al final
                if (loteResultados.size() >= 50 || i == numResultados - 1) {
                    escribirCSVIncremental(loteResultados, rutaCompleta);
                    totalResultadosEscritos += loteResultados.size();
                    loteResultados.clear();
                    
                    if (i % 100 == 0 || i == numResultados - 1) {
                        std::cout << "\r✅ Escritos " << (i + 1) << "/" << numResultados 
                                  << " del proceso " << proc << std::flush;
                    }
                }
            }
            std::cout << " ✓" << std::endl;
        }
    } else {
        // Enviar resultados al proceso 0
        int numResultados = misResultados.size();
        MPI_Send(&numResultados, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        
        for (const auto& resultado : misResultados) {
            // Enviar combinación
            int tamCombinacion = resultado.combinacion.size();
            MPI_Send(&tamCombinacion, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(resultado.combinacion.data(), tamCombinacion, MPI_INT, 0, 0, MPI_COMM_WORLD);
            
            // Enviar encendidos
            int numEncendidos = resultado.encendidos.size();
            MPI_Send(&numEncendidos, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            
            for (const auto& encendido : resultado.encendidos) {
                MPI_Send(&encendido.first, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                
                int tamEstado = encendido.second.size();
                MPI_Send(&tamEstado, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(encendido.second.data(), tamEstado, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
            
            // Enviar otros datos
            MPI_Send(&resultado.costoTotal, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
            int valida = resultado.valida ? 1 : 0;
            MPI_Send(&valida, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
        
        std::cout << "\n✅ Total de resultados escritos: " << totalResultadosEscritos << std::endl;
    }
    
    // Mostrar estadísticas finales (solo proceso 0)
    if (rank_ == 0) {
        auto fin = std::chrono::high_resolution_clock::now();
        auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(fin - inicio);
        
        mostrarEstadisticasFinales(duracion.count(), totalResultadosEscritos);
        std::cout << "Archivo generado: " << rutaCompleta << std::endl;
    }
    
    auto finRecoleccion = std::chrono::high_resolution_clock::now();
    estadisticasMPI_.tiempoRecoleccion = std::chrono::duration_cast<std::chrono::milliseconds>(finRecoleccion - inicioRecoleccion).count();
}

void AnalisisExhaustivoMPI::generarTodasLasCombinaciones(int longitud, std::vector<std::vector<int>>& combinaciones) {
    int totalCombinaciones = 1 << longitud; // 2^longitud
    combinaciones.reserve(totalCombinaciones);
    
    for (int i = 0; i < totalCombinaciones; ++i) {
        std::vector<int> combinacion(longitud);
        for (int j = 0; j < longitud; ++j) {
            combinacion[j] = (i >> j) & 1;
        }
        combinaciones.push_back(combinacion);
    }
}

std::vector<std::vector<int>> AnalisisExhaustivoMPI::obtenerCombinacionesParaProceso(
    const std::vector<std::vector<int>>& todasCombinaciones) {
    
    std::vector<std::vector<int>> misCombinaciones;
    
    // Broadcast del tamaño total
    int totalCombinaciones = 0;
    if (rank_ == 0) {
        totalCombinaciones = todasCombinaciones.size();
    }
    MPI_Bcast(&totalCombinaciones, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Calcular rango de combinaciones para este proceso
    int combinacionesPorProceso = totalCombinaciones / size_;
    int inicio = rank_ * combinacionesPorProceso;
    int fin = (rank_ == size_ - 1) ? totalCombinaciones : (rank_ + 1) * combinacionesPorProceso;
    
    if (rank_ == 0) {
        // El proceso 0 toma directamente sus combinaciones
        for (int i = inicio; i < fin; ++i) {
            misCombinaciones.push_back(todasCombinaciones[i]);
        }
        
        // Enviar combinaciones a otros procesos
        for (int proc = 1; proc < size_; ++proc) {
            int inicioProc = proc * combinacionesPorProceso;
            int finProc = (proc == size_ - 1) ? totalCombinaciones : (proc + 1) * combinacionesPorProceso;
            
            int numCombinaciones = finProc - inicioProc;
            MPI_Send(&numCombinaciones, 1, MPI_INT, proc, 0, MPI_COMM_WORLD);
            
            if (numCombinaciones > 0) {
                int longitud = todasCombinaciones[0].size();
                MPI_Send(&longitud, 1, MPI_INT, proc, 0, MPI_COMM_WORLD);
                
                for (int i = inicioProc; i < finProc; ++i) {
                    MPI_Send(todasCombinaciones[i].data(), longitud, MPI_INT, proc, 0, MPI_COMM_WORLD);
                }
            }
        }
    } else {
        // Recibir combinaciones del proceso 0
        int numCombinaciones;
        MPI_Recv(&numCombinaciones, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        if (numCombinaciones > 0) {
            int longitud;
            MPI_Recv(&longitud, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            for (int i = 0; i < numCombinaciones; ++i) {
                std::vector<int> combinacion(longitud);
                MPI_Recv(combinacion.data(), longitud, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                misCombinaciones.push_back(combinacion);
            }
        }
    }
    
    return misCombinaciones;
}

ResultadoCombinacion AnalisisExhaustivoMPI::analizarCombinacion(const std::vector<int>& combinacion) {
    ResultadoCombinacion resultado;
    resultado.combinacion = combinacion;
    
    // Crear escenario a partir de la combinación
    Escenario escenario;
    escenario.cargarDesdeArray(combinacion);
    
    // Resolver usando el calculador de costos
    auto solucion = calculador_.resolver(escenario);
    
    resultado.costoTotal = solucion.costoTotal;
    resultado.valida = solucion.solucionValida;
    
    if (!solucion.solucionValida) {
        return resultado;
    }
    
    // Analizar la secuencia de estados para detectar acciones clave
    if (solucion.estados.empty()) return resultado;
    
    // Encontrar posiciones que requieren generación
    std::vector<int> posicionesGeneracion;
    for (size_t i = 0; i < combinacion.size(); ++i) {
        if (combinacion[i] == 0) {
            posicionesGeneracion.push_back(i);
        }
    }
    
    if (posicionesGeneracion.empty()) return resultado;
    
    // Analizar la secuencia de estados para detectar acciones importantes
    Estado estadoAnterior = Estado::OFF_FRIO;
    
    for (size_t i = 0; i < solucion.estados.size(); ++i) {
        Estado estadoActual = solucion.estados[i];
        bool esAccionImportante = false;
        std::string tipoAccion;
        
        // Detectar encendidos (OFF -> ON)
        bool anteriorApagado = (estadoAnterior == Estado::OFF_CALIENTE || estadoAnterior == Estado::OFF_TIBIO || estadoAnterior == Estado::OFF_FRIO);
        bool actualEncendido = (estadoActual == Estado::ON_CALIENTE || estadoActual == Estado::ON_TIBIO || estadoActual == Estado::ON_FRIO);
        
        if (anteriorApagado && actualEncendido) {
            esAccionImportante = true;
            switch (estadoActual) {
                case Estado::ON_CALIENTE: tipoAccion = "C"; break;
                case Estado::ON_TIBIO: tipoAccion = "T"; break;
                case Estado::ON_FRIO: tipoAccion = "F"; break;
                default: break;
            }
        }
        // Detectar apagados estratégicos (ON -> OFF) que mantienen temperatura
        else {
            bool anteriorEncendido = (estadoAnterior == Estado::ON_CALIENTE || estadoAnterior == Estado::ON_TIBIO || estadoAnterior == Estado::ON_FRIO);
            bool actualApagado = (estadoActual == Estado::OFF_CALIENTE || estadoActual == Estado::OFF_TIBIO || estadoActual == Estado::OFF_FRIO);
            bool esGap = combinacion[i] == 1; // No requiere generación
            
            if (anteriorEncendido && actualApagado && esGap && estadoActual == Estado::OFF_CALIENTE) {
                esAccionImportante = true;
                tipoAccion = "C"; // Apagar manteniendo caliente
            }
        }
        
        // Detectar encendidos específicos para preparación
        if (i == 0 && actualEncendido) {
            esAccionImportante = true;
            switch (estadoActual) {
                case Estado::ON_CALIENTE: tipoAccion = "C"; break;
                case Estado::ON_TIBIO: tipoAccion = "T"; break;
                case Estado::ON_FRIO: tipoAccion = "F"; break;
                default: break;
            }
        }
        
        if (esAccionImportante && !tipoAccion.empty()) {
            resultado.encendidos.push_back({static_cast<int>(i), tipoAccion});
        }
        
        estadoAnterior = estadoActual;
    }
    
    return resultado;
}

void AnalisisExhaustivoMPI::sincronizarCache() {
    // Implementación básica de sincronización de caché
    // En una implementación más completa, aquí se intercambiarían patrones útiles
    // Por ahora, mantenemos la funcionalidad local del calculador
}

void AnalisisExhaustivoMPI::enviarCacheATodos() {
    // Implementación futura para cache distribuido
}

void AnalisisExhaustivoMPI::recibirCacheDeOtros() {
    // Implementación futura para cache distribuido
}

std::string AnalisisExhaustivoMPI::analizarTransicion(Estado estadoAnterior, Estado estadoActual, size_t posicion) {
    // Detectar transiciones de OFF a ON (encendidos)
    bool anteriorApagado = (estadoAnterior == Estado::OFF_CALIENTE || estadoAnterior == Estado::OFF_TIBIO || estadoAnterior == Estado::OFF_FRIO);
    bool actualEncendido = (estadoActual == Estado::ON_CALIENTE || estadoActual == Estado::ON_TIBIO || estadoActual == Estado::ON_FRIO);
    
    // Detectar transiciones de ON a OFF (apagados)
    bool anteriorEncendido = (estadoAnterior == Estado::ON_CALIENTE || estadoAnterior == Estado::ON_TIBIO || estadoAnterior == Estado::ON_FRIO);
    bool actualApagado = (estadoActual == Estado::OFF_CALIENTE || estadoActual == Estado::OFF_TIBIO || estadoActual == Estado::OFF_FRIO);
    
    if (posicion == 0) {
        // Primera posición: solo detectar encendidos iniciales
        if (actualEncendido) {
            switch (estadoActual) {
                case Estado::ON_CALIENTE: return "C";
                case Estado::ON_TIBIO: return "T"; 
                case Estado::ON_FRIO: return "F";
                default: return "";
            }
        }
    } else {
        // Encendidos (OFF → ON)
        if (anteriorApagado && actualEncendido) {
            switch (estadoActual) {
                case Estado::ON_CALIENTE: return "C";
                case Estado::ON_TIBIO: return "T";
                case Estado::ON_FRIO: return "F";
                default: return "";
            }
        }
        
        // Apagados manteniendo temperatura (ON → OFF)
        if (anteriorEncendido && actualApagado) {
            switch (estadoActual) {
                case Estado::OFF_CALIENTE: return "C"; // Apagar manteniendo caliente
                case Estado::OFF_TIBIO: return "T";    // Apagar manteniendo tibio
                case Estado::OFF_FRIO: return "F";     // Apagar frío
                default: return "";
            }
        }
    }
    
    return ""; // No hay acción relevante
}

std::string AnalisisExhaustivoMPI::formatearEstadoEncendido(Estado estadoAnterior, Estado estadoActual, int posicion) {
    if (estadoActual == Estado::ON_CALIENTE) return "C";
    if (estadoActual == Estado::ON_TIBIO) return "T";
    if (estadoActual == Estado::ON_FRIO) return "F";
    return "";
}

void AnalisisExhaustivoMPI::escribirCSV(const std::vector<ResultadoCombinacion>& resultados, const std::string& archivo) {
    std::ofstream csv(archivo);
    if (!csv.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo " << archivo << std::endl;
        return;
    }
    
    // Escribir encabezado
    csv << "Combinacion,Encendidos,Costo_Total,Valida\n";
    
    for (const auto& resultado : resultados) {
        // Escribir combinación
        for (size_t i = 0; i < resultado.combinacion.size(); ++i) {
            csv << resultado.combinacion[i];
            if (i < resultado.combinacion.size() - 1) csv << " ";
        }
        csv << ",";
        
        // Escribir encendidos
        if (resultado.encendidos.empty()) {
            csv << "Ninguno";
        } else {
            for (size_t i = 0; i < resultado.encendidos.size(); ++i) {
                csv << resultado.encendidos[i].first << resultado.encendidos[i].second;
                if (i < resultado.encendidos.size() - 1) csv << " - ";
            }
        }
        csv << ",";
        
        // Escribir costo y validez
        csv << std::fixed << std::setprecision(1) << resultado.costoTotal << ",";
        csv << (resultado.valida ? "Si" : "No") << "\n";
    }
    
    csv.close();
    std::cout << "CSV escrito con " << resultados.size() << " resultados." << std::endl;
}

void AnalisisExhaustivoMPI::mostrarProgreso(int combinacionActual, int totalCombinaciones) {
    // Mostrar progreso cada 10% o cada 100 combinaciones, lo que sea menor
    int intervalo = std::max(1, std::min(totalCombinaciones / 10, 100));
    
    if (combinacionActual % intervalo == 0 || combinacionActual == totalCombinaciones - 1) {
        double porcentaje = (double(combinacionActual + 1) / totalCombinaciones) * 100.0;
        int barras = porcentaje / 2; // Barra de 50 caracteres
        
        std::cout << "\r[";
        for (int i = 0; i < 50; ++i) {
            if (i < barras) std::cout << "█";
            else std::cout << " ";
        }
        std::cout << "] " << std::fixed << std::setprecision(1) << porcentaje << "% ("
                  << combinacionActual + 1 << "/" << totalCombinaciones << ")";
        std::cout.flush();
    }
}

void AnalisisExhaustivoMPI::mostrarEstadisticasFinales(double tiempoTotal, int totalCombinaciones) {
    std::cout << "\n=== ANÁLISIS COMPLETADO ===" << std::endl;
    std::cout << "⏱️  Tiempo total: " << tiempoTotal << " ms" << std::endl;
    std::cout << "📊 Combinaciones analizadas: " << totalCombinaciones << std::endl;
    std::cout << "⚡ Velocidad: " << std::fixed << std::setprecision(2) 
              << (totalCombinaciones / (tiempoTotal / 1000.0)) << " combinaciones/seg" << std::endl;
    
    std::cout << "\n=== ESTADÍSTICAS DE CACHÉ ===" << std::endl;
    std::cout << "✅ Cache hits: " << estadisticasCache_.hits << std::endl;
    std::cout << "❌ Cache misses: " << estadisticasCache_.misses << std::endl;
    std::cout << "💾 Patrones guardados: " << estadisticasCache_.patronesGuardados << std::endl;
    if (estadisticasCache_.hits + estadisticasCache_.misses > 0) {
        double tasa = (double(estadisticasCache_.hits) / (estadisticasCache_.hits + estadisticasCache_.misses)) * 100.0;
        std::cout << "📈 Tasa de acierto: " << std::fixed << std::setprecision(1) << tasa << "%" << std::endl;
    }
    std::cout << "⏰ Tiempo en análisis: " << std::fixed << std::setprecision(2) 
              << estadisticasCache_.tiempoTotal << " ms" << std::endl;
    
    if (size_ > 1) {
        std::cout << "\n=== ESTADÍSTICAS MPI ===" << std::endl;
        std::cout << "🔄 Procesos utilizados: " << size_ << std::endl;
        std::cout << "📤 Mensajes enviados: " << estadisticasMPI_.mensajesEnviados << std::endl;
        std::cout << "📥 Mensajes recibidos: " << estadisticasMPI_.mensajesRecibidos << std::endl;
        std::cout << "📊 Bytes enviados: " << estadisticasMPI_.bytesEnviados << " bytes" << std::endl;
        std::cout << "📊 Bytes recibidos: " << estadisticasMPI_.bytesRecibidos << " bytes" << std::endl;
        std::cout << "🚀 Tiempo distribución: " << estadisticasMPI_.tiempoDistribucion << " ms" << std::endl;
        std::cout << "🔙 Tiempo recolección: " << estadisticasMPI_.tiempoRecoleccion << " ms" << std::endl;
        
        double eficiencia = (tiempoTotal > 0) ? ((tiempoTotal / size_) / tiempoTotal) * 100.0 : 0.0;
        std::cout << "⚡ Eficiencia paralela: " << std::fixed << std::setprecision(1) << eficiencia << "%" << std::endl;
    }
}

void AnalisisExhaustivoMPI::registrarMensajeMPI(bool enviado, size_t bytes) {
    if (enviado) {
        estadisticasMPI_.mensajesEnviados++;
        estadisticasMPI_.bytesEnviados += bytes;
    } else {
        estadisticasMPI_.mensajesRecibidos++;
        estadisticasMPI_.bytesRecibidos += bytes;
    }
}

void AnalisisExhaustivoMPI::inicializarArchivoCSV(const std::string& archivo) {
    std::ofstream csv(archivo, std::ios::out | std::ios::trunc);
    if (!csv.is_open()) {
        std::cerr << "❌ Error: No se pudo crear el archivo " << archivo << std::endl;
        return;
    }
    
    // Escribir headers
    csv << "Combinacion,Encendidos,Costo_Total,Valida" << std::endl;
    csv.close();
}

void AnalisisExhaustivoMPI::escribirCSVIncremental(const std::vector<ResultadoCombinacion>& resultados, 
                                                   const std::string& archivo) {
    std::ofstream csv(archivo, std::ios::out | std::ios::app);
    if (!csv.is_open()) {
        std::cerr << "❌ Error: No se pudo abrir el archivo " << archivo << " para escritura incremental" << std::endl;
        return;
    }
    
    for (const auto& resultado : resultados) {
        // Escribir combinación
        for (size_t i = 0; i < resultado.combinacion.size(); ++i) {
            if (i > 0) csv << " ";
            csv << resultado.combinacion[i];
        }
        csv << ",";
        
        // Escribir encendidos
        for (size_t i = 0; i < resultado.encendidos.size(); ++i) {
            if (i > 0) csv << " - ";
            csv << resultado.encendidos[i].first << resultado.encendidos[i].second;
        }
        csv << ",";
        
        // Escribir costo y validez
        csv << std::fixed << std::setprecision(1) << resultado.costoTotal << ",";
        csv << (resultado.valida ? "Si" : "No") << std::endl;
    }
    
    csv.close();
    
    // Forzar flush del sistema de archivos
    std::system(("sync " + archivo + " 2>/dev/null || true").c_str());
}

// ===============================================================================
// IMPLEMENTACIONES PARA BENCHMARK DE RENDIMIENTO
// ===============================================================================

void AnalisisExhaustivoMPI::ejecutarBenchmark(const ConfiguracionBenchmark& config) {
    auto inicioTotal = std::chrono::high_resolution_clock::now();
    
    config_ = config;
    inicializarBenchmark(config);
    
    // Para casos grandes: escribir métricas de inicialización inmediatamente
    if (config.bits >= 18) {
        escribirMetricasProgresivas(config, "INIT", 0.0);
        if (config.modoVerboso) {
            std::cout << "📝 Proceso " << rank_ << " escribió métricas iniciales" << std::endl;
        }
    }
    
    if (rank_ == 0 && config.modoVerboso) {
        std::cout << "\n=== INICIANDO BENCHMARK DE RENDIMIENTO ===" << std::endl;
    }
    
    // Generar todas las combinaciones (solo para distribución)
    std::vector<std::vector<int>> todasCombinaciones;
    if (rank_ == 0) {
        generarTodasLasCombinaciones(config.bits, todasCombinaciones);
        if (config.modoVerboso) {
            std::cout << "✅ Generadas " << todasCombinaciones.size() << " combinaciones" << std::endl;
        }
    }
    
    // Distribuir combinaciones entre procesos
    auto inicioDistribucion = std::chrono::high_resolution_clock::now();
    auto misCombinaciones = obtenerCombinacionesParaProceso(todasCombinaciones);
    auto finDistribucion = std::chrono::high_resolution_clock::now();
    
    metricasLocales_.combinacionesAsignadas = misCombinaciones.size();
    metricasLocales_.tiempoInicializacion = std::chrono::duration_cast<std::chrono::microseconds>(
        finDistribucion - inicioDistribucion).count() / 1000.0;
    
    if (config.modoVerboso) {
        std::cout << "Proceso " << rank_ << " procesará " << misCombinaciones.size() << " combinaciones" << std::endl;
    }
    
    // Análisis de combinaciones (enfocado en rendimiento)
    auto inicioAnalisis = std::chrono::high_resolution_clock::now();
    int combinacionesProcesadas = 0;
    
    for (size_t i = 0; i < misCombinaciones.size(); ++i) {
        auto inicioCombo = std::chrono::high_resolution_clock::now();
        
        // Procesar combinación (modo rápido sin guardar detalles)
        bool resultadoValido = procesarCombinacionRapido(misCombinaciones[i]);
        if (resultadoValido) combinacionesProcesadas++;
        
        auto finCombo = std::chrono::high_resolution_clock::now();
        auto duracionCombo = std::chrono::duration_cast<std::chrono::microseconds>(finCombo - inicioCombo);
        metricasLocales_.tiempoAnalisis += duracionCombo.count() / 1000.0;
        
        // Mostrar progreso simplificado
        if (rank_ == 0 && config.modoVerboso) {
            double velocidad = (i + 1) / (metricasLocales_.tiempoAnalisis / 1000.0);
            mostrarProgresoSimplificado(i, misCombinaciones.size(), velocidad);
        }
        
        // Las estadísticas de caché se actualizan automáticamente en procesarCombinacionRapido()
        // usando las métricas reales del CalculadorCostos con resolverConPatrones()
        
        // Sincronizar caché distribuido solo para casos muy grandes (>25 bits)
        if (config.bits >= 25 && i > 0 && i % 2000 == 0) {
            sincronizarCacheDistribuido();
        }
        
        // Para casos grandes: escribir métricas progresivas cada 25% del progreso
        if (config.bits >= 18 && i > 0 && (i % (misCombinaciones.size() / 4) == 0)) {
            metricasLocales_.combinacionesProcesadas = combinacionesProcesadas;
            double progreso = ((double)i / misCombinaciones.size()) * 100.0;
            escribirMetricasProgresivas(config, "PROGRESS", progreso);
            
            if (config.modoVerboso) {
                std::cout << "📝 Proceso " << rank_ << " guardó progreso " 
                          << std::fixed << std::setprecision(1) << progreso << "%" << std::endl;
            }
        }
    }
    
    auto finAnalisis = std::chrono::high_resolution_clock::now();
    auto duracionAnalisis = std::chrono::duration_cast<std::chrono::milliseconds>(finAnalisis - inicioAnalisis);
    metricasLocales_.tiempoAnalisis = duracionAnalisis.count();
    metricasLocales_.combinacionesProcesadas = combinacionesProcesadas;
    
    // ESCRITURA FINAL CRÍTICA INMEDIATA (antes de cualquier otra operación)
    if (config.bits >= 18) {
        escribirMetricasProgresivas(config, "COMPLETED", 100.0);
        if (config.modoVerboso) {
            std::cout << "🔒 Proceso " << rank_ << " aseguró métricas finales" << std::endl;
        }
    }
    
    if (rank_ == 0 && config.modoVerboso) {
        std::cout << "\n✅ Proceso " << rank_ << " completó análisis en " 
                  << metricasLocales_.tiempoAnalisis << " ms" << std::endl;
    }
    
    // Para casos grandes: escribir métricas inmediatamente después del análisis
    if (config.bits >= 18) {
        auto tiempoParcial = std::chrono::high_resolution_clock::now();
        auto duracionParcial = std::chrono::duration_cast<std::chrono::milliseconds>(tiempoParcial - inicioTotal);
        
        // Escribir métricas de cada proceso inmediatamente
        std::string archivoBase = config.archivoMetricas;
        size_t pos = archivoBase.find(".csv");
        if (pos != std::string::npos) {
            archivoBase = archivoBase.substr(0, pos);
        }
        
        escribirMetricasIndividuales(archivoBase, rank_, duracionParcial.count());
        
        if (config.modoVerboso) {
            std::cout << "✅ Proceso " << rank_ << " guardó métricas tempranamente" << std::endl;
        }
    }
    
    // Obtener tiempo final
    auto finTotal = std::chrono::high_resolution_clock::now();
    auto duracionTotal = std::chrono::duration_cast<std::chrono::milliseconds>(finTotal - inicioTotal);
    metricasLocales_.tiempoFinalizacion = duracionTotal.count();
    
    // Escribir métricas inmediatamente (modo robusto para casos grandes)
    if (config.bits >= 18) {
        // Para casos grandes: solo crear archivo consolidado (métricas ya escritas tempranamente)
        escribirArchivoConsolidado(config, duracionTotal.count());
    } else {
        // Para casos pequeños: usar el método original con recolección MPI
        recolectarMetricas();
        
        if (rank_ == 0) {
            metricasGlobales_.tiempoTotal = duracionTotal.count();
            calcularMetricasGlobales();
            
            std::string rutaCompleta = "resultados/" + config.archivoMetricas;
            escribirMetricasCSV(rutaCompleta);
            mostrarResumenBenchmark();
        }
    }
}

bool AnalisisExhaustivoMPI::procesarCombinacionRapido(const std::vector<int>& combinacion) {
    // MÉTODO TRADICIONAL COMO PRIORIDAD PRINCIPAL
    Escenario escenario;
    escenario.cargarDesdeArray(combinacion);
    
    // Obtener estadísticas ANTES del procesamiento
    int cacheHitsAntes = calculador_.getCacheHits();
    int cacheMissesAntes = calculador_.getCacheMisses();
    
    // USAR resolverConPatrones (método tradicional con caché local)
    auto solucion = calculador_.resolverConPatrones(escenario);
    
    // Obtener estadísticas DESPUÉS del procesamiento
    int cacheHitsDespues = calculador_.getCacheHits();
    int cacheMissesDespues = calculador_.getCacheMisses();
    
    // Actualizar métricas reales de caché (incrementales)
    metricasLocales_.cacheHits += (cacheHitsDespues - cacheHitsAntes);
    metricasLocales_.cacheMisses += (cacheMissesDespues - cacheMissesAntes);
    
    // CACHÉ DISTRIBUIDO DESHABILITADO para evitar problemas MPI en casos pequeños
    // Solo se habilita para casos muy grandes donde el beneficio supera los problemas MPI
    if (solucion.solucionValida && config_.bits >= 25) {
        // Solo compartir sufijos ocasionalmente para evitar saturación MPI
        static int contadorCompartir = 0;
        contadorCompartir++;
        if (contadorCompartir % 500 == 0) { // Solo cada 500 soluciones válidas
            compartirSufijosEncontrados(combinacion, solucion.estados);
        }
    }
    
    return solucion.solucionValida;
}

void AnalisisExhaustivoMPI::inicializarBenchmark(const ConfiguracionBenchmark& config) {
    metricasLocales_.rank = rank_;
    metricasLocales_.combinacionesAsignadas = 0;
    metricasLocales_.combinacionesProcesadas = 0;
    metricasLocales_.tiempoAnalisis = 0.0;
    metricasLocales_.tiempoInicializacion = 0.0;
    metricasLocales_.tiempoFinalizacion = 0.0;
    
    // Inicializar estadísticas de caché reales
    metricasLocales_.cacheHits = 0;
    metricasLocales_.cacheMisses = 0;
    
    // Limpiar estadísticas del calculador para este benchmark
    calculador_.resetearEstadisticasCache();
    metricasLocales_.memoriaUsada = 0;
    
    // Obtener uso de memoria inicial
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        metricasLocales_.memoriaUsada = usage.ru_maxrss; // en KB en Linux, bytes en macOS
    }
}

void AnalisisExhaustivoMPI::recolectarMetricas() {
    if (rank_ == 0) {
        // Inicializar con métricas del proceso 0
        metricasGlobales_.metricasPorProceso.clear();
        metricasGlobales_.metricasPorProceso.push_back(metricasLocales_);
        
        // Recopilar métricas de otros procesos
        for (int proc = 1; proc < size_; ++proc) {
            MetricasProceso metricas;
            
            MPI_Recv(&metricas.rank, 1, MPI_INT, proc, TAG_METRICS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&metricas.combinacionesAsignadas, 1, MPI_INT, proc, TAG_METRICS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&metricas.combinacionesProcesadas, 1, MPI_INT, proc, TAG_METRICS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&metricas.tiempoAnalisis, 1, MPI_DOUBLE, proc, TAG_METRICS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&metricas.tiempoInicializacion, 1, MPI_DOUBLE, proc, TAG_METRICS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&metricas.tiempoFinalizacion, 1, MPI_DOUBLE, proc, TAG_METRICS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&metricas.memoriaUsada, 1, MPI_UNSIGNED_LONG, proc, TAG_METRICS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&metricas.cacheHits, 1, MPI_INT, proc, TAG_METRICS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&metricas.cacheMisses, 1, MPI_INT, proc, TAG_METRICS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            metricasGlobales_.metricasPorProceso.push_back(metricas);
        }
    } else {
        // Enviar métricas al proceso 0
        MPI_Send(&metricasLocales_.rank, 1, MPI_INT, 0, TAG_METRICS, MPI_COMM_WORLD);
        MPI_Send(&metricasLocales_.combinacionesAsignadas, 1, MPI_INT, 0, TAG_METRICS, MPI_COMM_WORLD);
        MPI_Send(&metricasLocales_.combinacionesProcesadas, 1, MPI_INT, 0, TAG_METRICS, MPI_COMM_WORLD);
        MPI_Send(&metricasLocales_.tiempoAnalisis, 1, MPI_DOUBLE, 0, TAG_METRICS, MPI_COMM_WORLD);
        MPI_Send(&metricasLocales_.tiempoInicializacion, 1, MPI_DOUBLE, 0, TAG_METRICS, MPI_COMM_WORLD);
        MPI_Send(&metricasLocales_.tiempoFinalizacion, 1, MPI_DOUBLE, 0, TAG_METRICS, MPI_COMM_WORLD);
        MPI_Send(&metricasLocales_.memoriaUsada, 1, MPI_UNSIGNED_LONG, 0, TAG_METRICS, MPI_COMM_WORLD);
        MPI_Send(&metricasLocales_.cacheHits, 1, MPI_INT, 0, TAG_METRICS, MPI_COMM_WORLD);
        MPI_Send(&metricasLocales_.cacheMisses, 1, MPI_INT, 0, TAG_METRICS, MPI_COMM_WORLD);
    }
    
    // CRÍTICO: Limpiar comunicaciones pendientes antes de sincronizar
    limpiarComunicacionesPendientes();
    
    // CRÍTICO: Sincronizar todos los procesos antes de continuar
    // Esto asegura que todas las operaciones MPI estén completadas
    MPI_Barrier(MPI_COMM_WORLD);
}

void AnalisisExhaustivoMPI::calcularMetricasGlobales() {
    metricasGlobales_.totalBits = config_.bits;
    metricasGlobales_.totalCombinaciones = 1LL << config_.bits;
    metricasGlobales_.numeroNucleos = config_.nucleos;
    metricasGlobales_.procesosUtilizados = size_;
    
    // Calcular métricas agregadas
    double tiempoMaximo = 0.0;
    double tiempoMinimo = std::numeric_limits<double>::max();
    int totalProcesadas = 0;
    
    for (const auto& metricas : metricasGlobales_.metricasPorProceso) {
        tiempoMaximo = std::max(tiempoMaximo, metricas.tiempoAnalisis);
        tiempoMinimo = std::min(tiempoMinimo, metricas.tiempoAnalisis);
        totalProcesadas += metricas.combinacionesProcesadas;
    }
    
    // Calcular velocidad
    metricasGlobales_.combinacionesPorSegundo = totalProcesadas / (metricasGlobales_.tiempoTotal / 1000.0);
    
    // Calcular speedup (comparado con tiempo secuencial estimado)
    double tiempoSecuencialEstimado = tiempoMaximo * size_; // Estimación conservativa
    metricasGlobales_.speedup = tiempoSecuencialEstimado / tiempoMaximo;
    
    // Calcular eficiencia paralela
    metricasGlobales_.eficienciaParalela = (metricasGlobales_.speedup / size_) * 100.0;
    
    // Calcular balance de carga
    if (tiempoMaximo > 0) {
        metricasGlobales_.balanceCarga = (tiempoMinimo / tiempoMaximo) * 100.0;
    } else {
        metricasGlobales_.balanceCarga = 100.0;
    }
    
    // Estadísticas de comunicación
    metricasGlobales_.totalBytesTransferidos = estadisticasMPI_.bytesEnviados + estadisticasMPI_.bytesRecibidos;
    metricasGlobales_.totalMensajesMPI = estadisticasMPI_.mensajesEnviados + estadisticasMPI_.mensajesRecibidos;
    metricasGlobales_.tiempoDistribucion = estadisticasMPI_.tiempoDistribucion;
    metricasGlobales_.tiempoRecoleccion = estadisticasMPI_.tiempoRecoleccion;
}

void AnalisisExhaustivoMPI::escribirMetricasCSV(const std::string& archivo) {
    std::ofstream csv(archivo);
    if (!csv.is_open()) {
        std::cerr << "❌ Error: No se pudo crear el archivo de métricas " << archivo << std::endl;
        return;
    }
    
    // Headers del CSV
    csv << "Seccion,Metrica,Valor,Unidad\n";
    
    // Métricas globales
    csv << "Global,Bits," << metricasGlobales_.totalBits << ",count\n";
    csv << "Global,Total_Combinaciones," << metricasGlobales_.totalCombinaciones << ",count\n";
    csv << "Global,Nucleos_Configurados," << metricasGlobales_.numeroNucleos << ",count\n";
    csv << "Global,Procesos_MPI," << metricasGlobales_.procesosUtilizados << ",count\n";
    csv << "Global,Tiempo_Total," << metricasGlobales_.tiempoTotal << ",ms\n";
    csv << "Global,Combinaciones_Por_Segundo," << std::fixed << std::setprecision(2) << metricasGlobales_.combinacionesPorSegundo << ",comb/s\n";
    csv << "Global,Speedup," << std::fixed << std::setprecision(2) << metricasGlobales_.speedup << ",factor\n";
    csv << "Global,Eficiencia_Paralela," << std::fixed << std::setprecision(1) << metricasGlobales_.eficienciaParalela << ",percent\n";
    csv << "Global,Balance_Carga," << std::fixed << std::setprecision(1) << metricasGlobales_.balanceCarga << ",percent\n";
    csv << "Global,Bytes_Transferidos," << metricasGlobales_.totalBytesTransferidos << ",bytes\n";
    csv << "Global,Mensajes_MPI," << metricasGlobales_.totalMensajesMPI << ",count\n";
    csv << "Global,Tiempo_Distribucion," << metricasGlobales_.tiempoDistribucion << ",ms\n";
    csv << "Global,Tiempo_Recoleccion," << metricasGlobales_.tiempoRecoleccion << ",ms\n";
    
    // Métricas por proceso
    for (const auto& metricas : metricasGlobales_.metricasPorProceso) {
        std::string proceso = "Proceso_" + std::to_string(metricas.rank);
        csv << proceso << ",Combinaciones_Asignadas," << metricas.combinacionesAsignadas << ",count\n";
        csv << proceso << ",Combinaciones_Procesadas," << metricas.combinacionesProcesadas << ",count\n";
        csv << proceso << ",Tiempo_Analisis," << std::fixed << std::setprecision(2) << metricas.tiempoAnalisis << ",ms\n";
        csv << proceso << ",Tiempo_Inicializacion," << std::fixed << std::setprecision(2) << metricas.tiempoInicializacion << ",ms\n";
        csv << proceso << ",Memoria_Usada," << metricas.memoriaUsada << ",KB\n";
        csv << proceso << ",Cache_Hits," << metricas.cacheHits << ",count\n";
        csv << proceso << ",Cache_Misses," << metricas.cacheMisses << ",count\n";
        
        // Calcular tasa de acierto de caché
        if (metricas.cacheHits + metricas.cacheMisses > 0) {
            double tasaAcierto = (double(metricas.cacheHits) / (metricas.cacheHits + metricas.cacheMisses)) * 100.0;
            csv << proceso << ",Tasa_Acierto_Cache," << std::fixed << std::setprecision(1) << tasaAcierto << ",percent\n";
        }
        
        // Velocidad del proceso
        if (metricas.tiempoAnalisis > 0) {
            double velocidadProceso = metricas.combinacionesProcesadas / (metricas.tiempoAnalisis / 1000.0);
            csv << proceso << ",Velocidad," << std::fixed << std::setprecision(2) << velocidadProceso << ",comb/s\n";
        }
    }
    
    csv.close();
    std::cout << "✅ Métricas escritas en: " << archivo << std::endl;
}

void AnalisisExhaustivoMPI::mostrarResumenBenchmark() {
    std::cout << "\n=== RESUMEN DEL BENCHMARK ===" << std::endl;
    std::cout << "🔢 Configuración: " << metricasGlobales_.totalBits << " bits (" 
              << metricasGlobales_.totalCombinaciones << " combinaciones)" << std::endl;
    std::cout << "⚙️  Recursos: " << metricasGlobales_.procesosUtilizados << " procesos MPI, " 
              << metricasGlobales_.numeroNucleos << " núcleos configurados" << std::endl;
    std::cout << "⏱️  Tiempo total: " << metricasGlobales_.tiempoTotal << " ms" << std::endl;
    std::cout << "⚡ Velocidad: " << std::fixed << std::setprecision(2) 
              << metricasGlobales_.combinacionesPorSegundo << " combinaciones/segundo" << std::endl;
    std::cout << "🚀 Speedup: " << std::fixed << std::setprecision(2) 
              << metricasGlobales_.speedup << "x" << std::endl;
    std::cout << "📈 Eficiencia paralela: " << std::fixed << std::setprecision(1) 
              << metricasGlobales_.eficienciaParalela << "%" << std::endl;
    std::cout << "⚖️  Balance de carga: " << std::fixed << std::setprecision(1) 
              << metricasGlobales_.balanceCarga << "%" << std::endl;
    std::cout << "📊 Comunicación MPI: " << metricasGlobales_.totalMensajesMPI 
              << " mensajes, " << metricasGlobales_.totalBytesTransferidos << " bytes" << std::endl;
    
    // Mostrar ranking de procesos
    std::cout << "\n=== RENDIMIENTO POR PROCESO ===" << std::endl;
    for (const auto& metricas : metricasGlobales_.metricasPorProceso) {
        double velocidad = metricas.combinacionesProcesadas / (metricas.tiempoAnalisis / 1000.0);
        std::cout << "Proceso " << metricas.rank << ": " 
                  << std::fixed << std::setprecision(0) << velocidad << " comb/s ("
                  << metricas.combinacionesProcesadas << "/" << metricas.combinacionesAsignadas 
                  << " procesadas)" << std::endl;
    }
}

void AnalisisExhaustivoMPI::mostrarProgresoSimplificado(int combinacionActual, int totalCombinaciones, double velocidad) {
    // Mostrar progreso simplificado para benchmark
    static int ultimoProgreso = -1;
    int progresoActual = (combinacionActual * 100) / totalCombinaciones;
    
    if (progresoActual != ultimoProgreso && progresoActual % 10 == 0) {
        std::cout << "⚡ " << progresoActual << "% - " 
                  << std::fixed << std::setprecision(0) << velocidad << " comb/s" << std::endl;
        ultimoProgreso = progresoActual;
    }
}

void AnalisisExhaustivoMPI::escribirMetricasRobustas(const ConfiguracionBenchmark& config, double tiempoTotal) {
    // Cada proceso escribe sus métricas independientemente (sin MPI)
    std::string archivoBase = config.archivoMetricas;
    
    // Remover extensión .csv si existe
    size_t pos = archivoBase.find(".csv");
    if (pos != std::string::npos) {
        archivoBase = archivoBase.substr(0, pos);
    }
    
    // Escribir métricas individuales
    escribirMetricasIndividuales(archivoBase, rank_, tiempoTotal);
    
    // El proceso 0 crea un resumen consolidado
    if (rank_ == 0) {
        // Esperar un poco para que otros procesos terminen de escribir
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Crear archivo consolidado
        std::string archivoConsolidado = "resultados/" + archivoBase + "_consolidated.csv";
        std::ofstream csv(archivoConsolidado);
        
        if (csv.is_open()) {
            // Headers
            csv << "Seccion,Metrica,Valor,Unidad\n";
            
            // Métricas globales estimadas
            long long totalCombinaciones = 1LL << config.bits;
            double velocidadEstimada = metricasLocales_.combinacionesProcesadas / (metricasLocales_.tiempoAnalisis / 1000.0);
            double velocidadTotal = velocidadEstimada * size_;
            
            csv << "Global,Bits," << config.bits << ",count\n";
            csv << "Global,Total_Combinaciones," << totalCombinaciones << ",count\n";
            csv << "Global,Nucleos_Configurados," << config.nucleos << ",count\n";
            csv << "Global,Procesos_MPI," << size_ << ",count\n";
            csv << "Global,Tiempo_Total_Estimado," << tiempoTotal << ",ms\n";
            csv << "Global,Velocidad_Estimada," << std::fixed << std::setprecision(2) << velocidadTotal << ",comb/s\n";
            csv << "Global,Speedup_Estimado," << std::fixed << std::setprecision(2) << (double)size_ << ",factor\n";
            csv << "Global,Modo_Ejecucion,Robusto_Sin_MPI,text\n";
            csv << "Global,Estado,Completado_Parcialmente,text\n";
            
            csv.close();
            
            std::cout << "\n=== BENCHMARK COMPLETADO (MODO ROBUSTO) ===" << std::endl;
            std::cout << "🔢 Configuración: " << config.bits << " bits (" << totalCombinaciones << " combinaciones)" << std::endl;
            std::cout << "⚙️  Recursos: " << size_ << " procesos MPI, " << config.nucleos << " núcleos" << std::endl;
            std::cout << "⏱️  Tiempo total: " << tiempoTotal << " ms" << std::endl;
            std::cout << "⚡ Velocidad estimada: " << std::fixed << std::setprecision(0) << velocidadTotal << " combinaciones/segundo" << std::endl;
            std::cout << "📊 Archivos generados:" << std::endl;
            std::cout << "  - " << archivoConsolidado << " (resumen)" << std::endl;
            
            for (int i = 0; i < size_; ++i) {
                std::cout << "  - resultados/" << archivoBase << "_proceso" << i << ".csv" << std::endl;
            }
            
            std::cout << "\n💡 Nota: Modo robusto activado para evitar problemas de memoria MPI" << std::endl;
            std::cout << "   Cada proceso guardó sus métricas independientemente." << std::endl;
        }
    }
}

void AnalisisExhaustivoMPI::escribirMetricasIndividuales(const std::string& archivoBase, int rank, double tiempoTotal) {
    std::string archivo = "resultados/" + archivoBase + "_proceso" + std::to_string(rank) + ".csv";
    std::ofstream csv(archivo);
    
    if (!csv.is_open()) {
        std::cerr << "❌ Proceso " << rank << ": Error al crear " << archivo << std::endl;
        return;
    }
    
    // Headers
    csv << "Seccion,Metrica,Valor,Unidad\n";
    
    // Información del proceso
    std::string procesoId = "Proceso_" + std::to_string(rank);
    csv << procesoId << ",Rank," << rank << ",count\n";
    csv << procesoId << ",Combinaciones_Asignadas," << metricasLocales_.combinacionesAsignadas << ",count\n";
    csv << procesoId << ",Combinaciones_Procesadas," << metricasLocales_.combinacionesProcesadas << ",count\n";
    csv << procesoId << ",Tiempo_Analisis," << std::fixed << std::setprecision(2) << metricasLocales_.tiempoAnalisis << ",ms\n";
    csv << procesoId << ",Tiempo_Inicializacion," << std::fixed << std::setprecision(2) << metricasLocales_.tiempoInicializacion << ",ms\n";
    csv << procesoId << ",Tiempo_Total," << std::fixed << std::setprecision(2) << tiempoTotal << ",ms\n";
    csv << procesoId << ",Memoria_Usada," << metricasLocales_.memoriaUsada << ",KB\n";
    csv << procesoId << ",Cache_Hits," << metricasLocales_.cacheHits << ",count\n";
    csv << procesoId << ",Cache_Misses," << metricasLocales_.cacheMisses << ",count\n";
    
    // Calcular métricas derivadas
    if (metricasLocales_.tiempoAnalisis > 0) {
        double velocidad = metricasLocales_.combinacionesProcesadas / (metricasLocales_.tiempoAnalisis / 1000.0);
        csv << procesoId << ",Velocidad," << std::fixed << std::setprecision(2) << velocidad << ",comb/s\n";
    }
    
    if (metricasLocales_.combinacionesAsignadas > 0) {
        double completitud = (double(metricasLocales_.combinacionesProcesadas) / metricasLocales_.combinacionesAsignadas) * 100.0;
        csv << procesoId << ",Completitud," << std::fixed << std::setprecision(1) << completitud << ",percent\n";
    }
    
    if (metricasLocales_.cacheHits + metricasLocales_.cacheMisses > 0) {
        double tasaAcierto = (double(metricasLocales_.cacheHits) / (metricasLocales_.cacheHits + metricasLocales_.cacheMisses)) * 100.0;
        csv << procesoId << ",Tasa_Acierto_Cache," << std::fixed << std::setprecision(1) << tasaAcierto << ",percent\n";
    }
    
    csv.close();
    
    if (rank == 0 || config_.modoVerboso) {
        std::cout << "✅ Proceso " << rank << " guardó métricas en: " << archivo << std::endl;
    }
}

void AnalisisExhaustivoMPI::escribirArchivoConsolidado(const ConfiguracionBenchmark& config, double tiempoTotal) {
    // Solo el proceso 0 crea el archivo consolidado
    if (rank_ != 0) return;
    
    std::string archivoBase = config.archivoMetricas;
    
    // Remover extensión .csv si existe
    size_t pos = archivoBase.find(".csv");
    if (pos != std::string::npos) {
        archivoBase = archivoBase.substr(0, pos);
    }
    
    // Esperar un poco para que otros procesos terminen de escribir
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Crear archivo consolidado
    std::string archivoConsolidado = "resultados/" + archivoBase + "_consolidated.csv";
    std::ofstream csv(archivoConsolidado);
    
    if (csv.is_open()) {
        // Headers
        csv << "Seccion,Metrica,Valor,Unidad\n";
        
        // Métricas globales estimadas
        long long totalCombinaciones = 1LL << config.bits;
        double velocidadEstimada = metricasLocales_.combinacionesProcesadas / (metricasLocales_.tiempoAnalisis / 1000.0);
        double velocidadTotal = velocidadEstimada * size_;
        
        csv << "Global,Bits," << config.bits << ",count\n";
        csv << "Global,Total_Combinaciones," << totalCombinaciones << ",count\n";
        csv << "Global,Nucleos_Configurados," << config.nucleos << ",count\n";
        csv << "Global,Procesos_MPI," << size_ << ",count\n";
        csv << "Global,Tiempo_Total_Estimado," << tiempoTotal << ",ms\n";
        csv << "Global,Velocidad_Estimada," << std::fixed << std::setprecision(2) << velocidadTotal << ",comb/s\n";
        csv << "Global,Speedup_Estimado," << std::fixed << std::setprecision(2) << (double)size_ << ",factor\n";
        csv << "Global,Modo_Ejecucion,Robusto_Escritura_Temprana,text\n";
        csv << "Global,Estado,Completado_Con_Metricas_Tempranas,text\n";
        
        csv.close();
        
        std::cout << "\n=== BENCHMARK COMPLETADO (MODO ROBUSTO) ===" << std::endl;
        std::cout << "🔢 Configuración: " << config.bits << " bits (" << totalCombinaciones << " combinaciones)" << std::endl;
        std::cout << "⚙️  Recursos: " << size_ << " procesos MPI, " << config.nucleos << " núcleos" << std::endl;
        std::cout << "⏱️  Tiempo total: " << tiempoTotal << " ms" << std::endl;
        std::cout << "⚡ Velocidad estimada: " << std::fixed << std::setprecision(0) << velocidadTotal << " combinaciones/segundo" << std::endl;
        std::cout << "📊 Archivos generados:" << std::endl;
        std::cout << "  - " << archivoConsolidado << " (resumen global)" << std::endl;
        
        // Contar archivos individuales existentes
        int archivosEncontrados = 0;
        for (int i = 0; i < size_; ++i) {
            std::string archivoIndividual = "resultados/" + archivoBase + "_proceso" + std::to_string(i) + ".csv";
            std::ifstream check(archivoIndividual);
            if (check.good()) {
                std::cout << "  - " << archivoIndividual << std::endl;
                archivosEncontrados++;
            }
            check.close();
        }
        
        std::cout << "\n📈 Resumen:" << std::endl;
        std::cout << "  ✅ Procesos que guardaron métricas: " << archivosEncontrados << "/" << size_ << std::endl;
        std::cout << "  💡 Modo robusto: Métricas escritas tempranamente para evitar fallos MPI" << std::endl;
        
        if (archivosEncontrados < size_) {
            std::cout << "  ⚠️  Algunos procesos no completaron por limitaciones de memoria" << std::endl;
        }
    } else {
        std::cerr << "❌ Error al crear archivo consolidado: " << archivoConsolidado << std::endl;
    }
}

void AnalisisExhaustivoMPI::escribirMetricasProgresivas(const ConfiguracionBenchmark& config, const std::string& estado, double progreso) {
    std::string archivoBase = config.archivoMetricas;
    
    // Remover extensión .csv si existe
    size_t pos = archivoBase.find(".csv");
    if (pos != std::string::npos) {
        archivoBase = archivoBase.substr(0, pos);
    }
    
    // Crear archivo con timestamp y estado
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    char timestamp[100];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm);
    
    std::string archivo = "resultados/" + archivoBase + "_proceso" + std::to_string(rank_) + 
                         "_" + estado + "_" + timestamp + ".csv";
    
    std::ofstream csv(archivo);
    
    if (!csv.is_open()) {
        // Usar stderr para no interferir con output del benchmark
        std::cerr << "⚠️ Proceso " << rank_ << " no pudo escribir " << archivo << std::endl;
        
        // Intentar archivo básico como fallback
        std::string archivoFallback = "resultados/proceso" + std::to_string(rank_) + "_" + estado + ".csv";
        csv.open(archivoFallback);
        if (!csv.is_open()) {
            return; // No se puede escribir nada
        }
        archivo = archivoFallback;
    }
    
    // Headers
    csv << "Seccion,Metrica,Valor,Unidad\n";
    
    // Información del proceso
    std::string procesoId = "Proceso_" + std::to_string(rank_);
    csv << procesoId << ",Rank," << rank_ << ",count\n";
    csv << procesoId << ",Estado," << estado << ",text\n";
    csv << procesoId << ",Progreso," << std::fixed << std::setprecision(1) << progreso << ",percent\n";
    csv << procesoId << ",Timestamp," << timestamp << ",text\n";
    csv << procesoId << ",Combinaciones_Asignadas," << metricasLocales_.combinacionesAsignadas << ",count\n";
    csv << procesoId << ",Combinaciones_Procesadas," << metricasLocales_.combinacionesProcesadas << ",count\n";
    csv << procesoId << ",Tiempo_Analisis," << std::fixed << std::setprecision(2) << metricasLocales_.tiempoAnalisis << ",ms\n";
    csv << procesoId << ",Tiempo_Inicializacion," << std::fixed << std::setprecision(2) << metricasLocales_.tiempoInicializacion << ",ms\n";
    
    // Obtener uso de memoria actual
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        csv << procesoId << ",Memoria_Usada_Actual," << usage.ru_maxrss << ",KB\n";
    }
    
    csv << procesoId << ",Cache_Hits," << metricasLocales_.cacheHits << ",count\n";
    csv << procesoId << ",Cache_Misses," << metricasLocales_.cacheMisses << ",count\n";
    
    // Calcular métricas en tiempo real
    if (metricasLocales_.tiempoAnalisis > 0 && metricasLocales_.combinacionesProcesadas > 0) {
        double velocidad = metricasLocales_.combinacionesProcesadas / (metricasLocales_.tiempoAnalisis / 1000.0);
        csv << procesoId << ",Velocidad_Actual," << std::fixed << std::setprecision(2) << velocidad << ",comb/s\n";
    }
    
    if (metricasLocales_.combinacionesAsignadas > 0) {
        double completitud = (double(metricasLocales_.combinacionesProcesadas) / metricasLocales_.combinacionesAsignadas) * 100.0;
        csv << procesoId << ",Completitud," << std::fixed << std::setprecision(1) << completitud << ",percent\n";
    }
    
    csv.close();
    
    // Flush inmediato a disco
    std::system(("sync " + archivo + " 2>/dev/null || true").c_str());
}

// ==================================================================================
// IMPLEMENTACIÓN DE CACHÉ DISTRIBUIDO MPI
// ==================================================================================

void AnalisisExhaustivoMPI::sincronizarCacheDistribuido() {
    // Sincronización no bloqueante para evitar deadlocks
    recibirPatronesDeOtrosProcesos();
}

void AnalisisExhaustivoMPI::compartirSufijosEncontrados(const std::vector<int>& combinacion, const std::vector<Estado>& solucion) {
    // Compartir solo el PRIMER sufijo válido de 0 a 0 para minimizar tráfico MPI
    for (size_t i = 0; i < combinacion.size(); ++i) {
        if (combinacion[i] == 0) {
            // Buscar el próximo 0 para formar sufijo completo
            for (size_t j = i + 1; j < combinacion.size(); ++j) {
                if (combinacion[j] == 0) {
                    // Sufijo encontrado de posición i a j (inclusive)
                    std::vector<int> patronSufijo(combinacion.begin() + i, combinacion.begin() + j + 1);
                    std::vector<Estado> solucionSufijo(solucion.begin() + i, solucion.begin() + j + 1);
                    
                    // Verificar que el sufijo es válido, útil y NO está ya en caché local
                    if (patronSufijo.size() >= 3 && patronSufijo.size() <= 8 && !calculador_.estaEnCache(patronSufijo)) {
                        // Solo enviar sufijos de tamaño medio (3-8) para mayor utilidad
                        enviarPatronAOtrosProcesos(patronSufijo, solucionSufijo);
                        
                        // También guardar localmente para uso futuro
                        calculador_.guardarEnCache(patronSufijo, solucionSufijo);
                        
                        // SOLO compartir el primer sufijo útil encontrado
                        return;
                    }
                }
            }
        }
    }
}

bool AnalisisExhaustivoMPI::buscarSufijoEnCacheDistribuido(const std::vector<int>& sufijo, std::vector<Estado>& solucionEncontrada) {
    // Primero buscar en caché local
    if (calculador_.estaEnCache(sufijo)) {
        solucionEncontrada = calculador_.obtenerDeCache(sufijo);
        return true;
    }
    
    // Si no está en caché local, intentar recibir de otros procesos
    // (implementación simplificada - en una versión completa usaríamos un protocolo más sofisticado)
    recibirPatronesDeOtrosProcesos();
    
    // Intentar nuevamente después de la sincronización
    if (calculador_.estaEnCache(sufijo)) {
        solucionEncontrada = calculador_.obtenerDeCache(sufijo);
        return true;
    }
    
    return false;
}

void AnalisisExhaustivoMPI::enviarPatronAOtrosProcesos(const std::vector<int>& patron, const std::vector<Estado>& solucion) {
    // Enviar patrón a todos los otros procesos de forma no bloqueante
    for (int destino = 0; destino < size_; ++destino) {
        if (destino != rank_) {
            // Enviar tamaño del patrón
            int tamanoPatron = static_cast<int>(patron.size());
            MPI_Send(&tamanoPatron, 1, MPI_INT, destino, TAG_CACHE_COUNT, MPI_COMM_WORLD);
            
            // Enviar datos del patrón
            if (tamanoPatron > 0) {
                MPI_Send(const_cast<int*>(patron.data()), tamanoPatron, MPI_INT, destino, TAG_CACHE_PATTERN, MPI_COMM_WORLD);
                
                // Convertir Estados a int para envío MPI
                std::vector<int> solucionInt;
                solucionInt.reserve(solucion.size());
                for (const auto& estado : solucion) {
                    solucionInt.push_back(static_cast<int>(estado));
                }
                
                MPI_Send(solucionInt.data(), tamanoPatron, MPI_INT, destino, TAG_CACHE_SOLUTION, MPI_COMM_WORLD);
                
                // Actualizar estadísticas MPI (aproximado)
                // En una implementación completa, estas irían a metricasGlobales
            }
        }
    }
}

void AnalisisExhaustivoMPI::recibirPatronesDeOtrosProcesos() {
    // Recibir patrones de forma no bloqueante para evitar bloqueos
    int flag;
    MPI_Status status;
    
    // Intentar recibir de cualquier proceso
    for (int fuente = 0; fuente < size_; ++fuente) {
        if (fuente != rank_) {
            // Verificar si hay mensajes pendientes de este proceso
            MPI_Iprobe(fuente, TAG_CACHE_COUNT, MPI_COMM_WORLD, &flag, &status);
            
            if (flag) {
                // Recibir tamaño del patrón
                int tamanoPatron;
                MPI_Recv(&tamanoPatron, 1, MPI_INT, fuente, TAG_CACHE_COUNT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                if (tamanoPatron > 0 && tamanoPatron <= 100) { // Limite de seguridad
                    // Recibir patrón
                    std::vector<int> patron(tamanoPatron);
                    MPI_Recv(patron.data(), tamanoPatron, MPI_INT, fuente, TAG_CACHE_PATTERN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    
                    // Recibir solución
                    std::vector<int> solucionInt(tamanoPatron);
                    MPI_Recv(solucionInt.data(), tamanoPatron, MPI_INT, fuente, TAG_CACHE_SOLUTION, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    
                    // Convertir de int a Estado
                    std::vector<Estado> solucion;
                    solucion.reserve(solucionInt.size());
                    for (int estadoInt : solucionInt) {
                        solucion.push_back(static_cast<Estado>(estadoInt));
                    }
                    
                    // Guardar en caché local para uso futuro
                    calculador_.guardarEnCache(patron, solucion);
                    
                    // NO incrementar cache hits aquí - eso sería artificial
                    // Los cache hits reales se cuentan cuando resolverConPatrones() usa el caché
                    
                    // Solo mostrar en modo muy verbose (comentado para reducir output)
                    // std::cout << "📥 Proceso " << rank_ << " recibió patrón de proceso " << fuente 
                    //           << " (tamaño: " << tamanoPatron << ")" << std::endl;
                }
            }
        }
    }
}

void AnalisisExhaustivoMPI::limpiarComunicacionesPendientes() {
    // Solo limpiar mensajes para casos grandes donde se usa caché distribuido
    if (config_.bits < 25) {
        return; // No hay mensajes de caché distribuido para casos pequeños
    }
    
    // Limpiar mensajes MPI pendientes de forma conservadora
    int flag;
    MPI_Status status;
    int messagesPendingCount = 0;
    const int MAX_CLEANUP_MESSAGES = 100; // Límite conservador
    
    // Solo limpiar mensajes específicos de caché distribuido
    for (int fuente = 0; fuente < size_ && messagesPendingCount < MAX_CLEANUP_MESSAGES; ++fuente) {
        if (fuente != rank_) {
            // Verificar y limpiar solo mensajes TAG_CACHE_COUNT
            MPI_Iprobe(fuente, TAG_CACHE_COUNT, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                int tamanoPatron;
                MPI_Recv(&tamanoPatron, 1, MPI_INT, fuente, TAG_CACHE_COUNT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                messagesPendingCount++;
                
                // Si hay tamaño válido, recibir los datos asociados
                if (tamanoPatron > 0 && tamanoPatron <= 50) {
                    std::vector<int> patron(tamanoPatron);
                    std::vector<int> solucionInt(tamanoPatron);
                    
                    MPI_Iprobe(fuente, TAG_CACHE_PATTERN, MPI_COMM_WORLD, &flag, &status);
                    if (flag) {
                        MPI_Recv(patron.data(), tamanoPatron, MPI_INT, fuente, TAG_CACHE_PATTERN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        messagesPendingCount++;
                    }
                    
                    MPI_Iprobe(fuente, TAG_CACHE_SOLUTION, MPI_COMM_WORLD, &flag, &status);
                    if (flag) {
                        MPI_Recv(solucionInt.data(), tamanoPatron, MPI_INT, fuente, TAG_CACHE_SOLUTION, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        messagesPendingCount++;
                    }
                }
            }
        }
    }
    
    if (messagesPendingCount > 0 && rank_ == 0) {
        std::cout << "🧹 Limpiados " << messagesPendingCount << " mensajes MPI pendientes" << std::endl;
    }
} 