#include "../include/cache_prefijos_mpi.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <sstream>

void CachePrefijos::habilitarLogging(bool habilitar) {
    logging_enabled = habilitar;
    if (logging_enabled && !log_cache.is_open()) {
        std::string nombre_archivo = "cache_log_proceso_" + std::to_string(rank) + ".txt";
        log_cache.open(nombre_archivo);
        if (log_cache.is_open()) {
            log_cache << "=== LOG DE CACHE DE PREFIJOS - PROCESO " << rank << " ===\n";
            log_cache << "Timestamp,Tipo,Patron,Longitud,Costo,TiempoMs,Frecuencia,Detalles\n";
            log_cache.flush();
        }
    }
}

void CachePrefijos::cerrarLog() {
    if (log_cache.is_open()) {
        log_cache << "\n=== FIN DEL LOG ===\n";
        log_cache.close();
    }
}

void CachePrefijos::escribirLog(const std::string& tipo, const std::string& patron, 
                               double costo, double tiempo, int frecuencia) {
    if (!logging_enabled || !log_cache.is_open()) return;
    
    auto ahora = std::chrono::steady_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        ahora.time_since_epoch()).count();
    
    log_cache << timestamp << "," 
              << tipo << "," 
              << patron << "," 
              << patron.length() << ","
              << std::fixed << std::setprecision(2) << costo << ","
              << std::fixed << std::setprecision(2) << tiempo << ","
              << frecuencia << ",";
    
    // Agregar detalles específicos según el tipo
    if (tipo == "CACHE_HIT") {
        log_cache << "Prefijo_reutilizado";
    } else if (tipo == "CACHE_MISS") {
        log_cache << "Nuevo_calculo_requerido";
    } else if (tipo == "CACHE_ADD") {
        log_cache << "Prefijo_agregado_tiempo>" << tiempo << "ms";
    } else if (tipo == "CACHE_SKIP") {
        log_cache << "No_cacheado_tiempo<" << umbral_tiempo_cache_ms << "ms";
    } else if (tipo == "ENVIADO") {
        log_cache << "Enviado_para_intercambio_freq=" << frecuencia;
    } else if (tipo == "RECIBIDO") {
        log_cache << "Recibido_de_otros_procesos";
    } else if (tipo == "LIMPIEZA") {
        log_cache << "Entrada_eliminada_por_limpieza";
    }
    
    log_cache << "\n";
    log_cache.flush();
}

// Función auxiliar para convertir estados a string
std::string estadosAString(const std::vector<EstadoMaquina>& estados) {
    std::string resultado;
    for (const auto& estado : estados) {
        switch (estado) {
            case EstadoMaquina::OFF_FRIO:
            case EstadoMaquina::OFF_TIBIO:
            case EstadoMaquina::OFF_CALIENTE: resultado += '0'; break;
            case EstadoMaquina::ON_FRIO: resultado += '1'; break;
            case EstadoMaquina::ON_TIBIO: resultado += '2'; break;
            case EstadoMaquina::ON_CALIENTE: resultado += '3'; break;
        }
    }
    return resultado;
}

// Función auxiliar para convertir string a estados
std::vector<EstadoMaquina> stringAEstados(const std::string& patron) {
    std::vector<EstadoMaquina> estados;
    for (char c : patron) {
        switch (c) {
            case '0': estados.push_back(EstadoMaquina::OFF_FRIO); break;
            case '1': estados.push_back(EstadoMaquina::ON_FRIO); break;
            case '2': estados.push_back(EstadoMaquina::ON_TIBIO); break;
            case '3': estados.push_back(EstadoMaquina::ON_CALIENTE); break;
        }
    }
    return estados;
}

bool CachePrefijos::buscarPrefijo(const std::string& patron, EntradaCache& resultado) {
    auto it = cache_local.find(patron);
    if (it != cache_local.end()) {
        resultado = it->second;
        // Actualizar estadísticas de uso
        it->second.frecuencia_uso++;
        it->second.ultimo_uso = std::chrono::steady_clock::now();
        hits_cache++;
        
        // Log del cache hit
        escribirLog("CACHE_HIT", patron, it->second.costo_acumulado, 
                   it->second.tiempo_computo_ms, it->second.frecuencia_uso);
        
        return true;
    }
    misses_cache++;
    
    // Log del cache miss
    escribirLog("CACHE_MISS", patron);
    
    return false;
}

void CachePrefijos::agregarPrefijo(const std::string& patron, 
                                  const std::vector<EstadoMaquina>& estados, 
                                  double costo, double tiempo_computo) {
    // Solo cachear si el tiempo de cómputo justifica el cache
    if (tiempo_computo < umbral_tiempo_cache_ms) {
        escribirLog("CACHE_SKIP", patron, costo, tiempo_computo);
        return;
    }
    
    // Limpiar cache si está lleno
    if (cache_local.size() >= max_cache_size) {
        limpiarCacheAntiguo();
    }
    
    // Agregar nueva entrada
    cache_local[patron] = EntradaCache(estados, costo, tiempo_computo);
    
    // Log del prefijo agregado
    escribirLog("CACHE_ADD", patron, costo, tiempo_computo, 1);
}

void CachePrefijos::limpiarCacheAntiguo() {
    // Crear vector de pares (tiempo_último_uso, patrón) para ordenar
    std::vector<std::pair<std::chrono::steady_clock::time_point, std::string>> entradas;
    
    for (const auto& par : cache_local) {
        entradas.push_back({par.second.ultimo_uso, par.first});
    }
    
    // Ordenar por tiempo de último uso (más antiguos primero)
    std::sort(entradas.begin(), entradas.end());
    
    // Eliminar el 25% más antiguo
    size_t a_eliminar = cache_local.size() / 4;
    for (size_t i = 0; i < a_eliminar && i < entradas.size(); i++) {
        const std::string& patron = entradas[i].second;
        auto it = cache_local.find(patron);
        if (it != cache_local.end()) {
            escribirLog("LIMPIEZA", patron, it->second.costo_acumulado, 
                       it->second.tiempo_computo_ms, it->second.frecuencia_uso);
            cache_local.erase(it);
        }
    }
    
    if (rank == 0) {
        std::cout << "Cache limpiado: eliminadas " << a_eliminar << " entradas antiguas\n";
    }
}

void CachePrefijos::intercambiarPrefijosValiosos() {
    // Paso 1: Recopilar prefijos valiosos de cada proceso
    std::vector<PrefijoMPI> prefijos_valiosos;
    
    // Filtrar prefijos locales que valen la pena compartir
    for (const auto& par : cache_local) {
        const std::string& patron = par.first;
        const EntradaCache& entrada = par.second;
        
        // Criterios para considerar un prefijo "valioso"
        if (entrada.frecuencia_uso >= min_frecuencia_intercambio && 
            entrada.tiempo_computo_ms >= umbral_tiempo_cache_ms) {
            
            PrefijoMPI prefijo_mpi;
            strncpy(prefijo_mpi.patron, patron.c_str(), sizeof(prefijo_mpi.patron) - 1);
            prefijo_mpi.costo = entrada.costo_acumulado;
            prefijo_mpi.frecuencia = entrada.frecuencia_uso;
            prefijo_mpi.tiempo_ms = entrada.tiempo_computo_ms;
            prefijo_mpi.rank_origen = rank;
            
            prefijos_valiosos.push_back(prefijo_mpi);
            
            // Log del prefijo siendo enviado
            escribirLog("ENVIADO", patron, entrada.costo_acumulado, 
                       entrada.tiempo_computo_ms, entrada.frecuencia_uso);
        }
    }
    
    // Paso 2: Enviar número de prefijos al proceso 0
    int num_prefijos_local = prefijos_valiosos.size();
    std::vector<int> num_prefijos_por_proceso;
    
    if (rank == 0) {
        num_prefijos_por_proceso.resize(size);
    }
    
    MPI_Gather(&num_prefijos_local, 1, MPI_INT, 
               num_prefijos_por_proceso.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Paso 3: Recopilar todos los prefijos en proceso 0
    std::vector<PrefijoMPI> todos_los_prefijos;
    std::vector<int> desplazamientos;
    int total_prefijos = 0;
    
    if (rank == 0) {
        desplazamientos.resize(size);
        for (int i = 0; i < size; i++) {
            desplazamientos[i] = total_prefijos;
            total_prefijos += num_prefijos_por_proceso[i];
        }
        todos_los_prefijos.resize(total_prefijos);
    }
    
    MPI_Gatherv(prefijos_valiosos.data(), num_prefijos_local, MPI_BYTE,
                todos_los_prefijos.data(), num_prefijos_por_proceso.data(),
                desplazamientos.data(), MPI_BYTE, 0, MPI_COMM_WORLD);
    
    // Paso 4: Proceso 0 filtra y selecciona los mejores
    std::vector<PrefijoMPI> prefijos_seleccionados;
    
    if (rank == 0) {
        // Ordenar por valor (frecuencia * tiempo_computo)
        std::sort(todos_los_prefijos.begin(), todos_los_prefijos.end(),
                  [](const PrefijoMPI& a, const PrefijoMPI& b) {
                      double valor_a = a.frecuencia * a.tiempo_ms;
                      double valor_b = b.frecuencia * b.tiempo_ms;
                      return valor_a > valor_b;
                  });
        
        // Seleccionar los top N (máximo 1000 o 10% del total)
        int max_a_distribuir = std::min(1000, std::max(1, total_prefijos / 10));
        
        for (int i = 0; i < std::min(max_a_distribuir, total_prefijos); i++) {
            prefijos_seleccionados.push_back(todos_los_prefijos[i]);
        }
        
        std::cout << "Intercambio de cache: recopilados " << total_prefijos 
                  << " prefijos, distribuyendo " << prefijos_seleccionados.size() << "\n";
    }
    
    // Paso 5: Distribuir prefijos seleccionados a todos los procesos
    int num_seleccionados = prefijos_seleccionados.size();
    MPI_Bcast(&num_seleccionados, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank != 0) {
        prefijos_seleccionados.resize(num_seleccionados);
    }
    
    if (num_seleccionados > 0) {
        MPI_Bcast(prefijos_seleccionados.data(), 
                  num_seleccionados * sizeof(PrefijoMPI), MPI_BYTE, 0, MPI_COMM_WORLD);
        
        // Paso 6: Cada proceso integra los prefijos recibidos
        for (const auto& prefijo_mpi : prefijos_seleccionados) {
            std::string patron(prefijo_mpi.patron);
            
            // Solo agregar si no existe o si el recibido es mejor
            auto it = cache_local.find(patron);
            if (it == cache_local.end()) {
                // Crear entrada desde el prefijo MPI recibido
                std::vector<EstadoMaquina> estados = stringAEstados(patron);
                EntradaCache nueva_entrada(estados, prefijo_mpi.costo, prefijo_mpi.tiempo_ms);
                nueva_entrada.frecuencia_uso = prefijo_mpi.frecuencia;
                cache_local[patron] = nueva_entrada;
                prefijos_recibidos++;
                
                // Log del prefijo recibido
                escribirLog("RECIBIDO", patron, prefijo_mpi.costo, 
                           prefijo_mpi.tiempo_ms, prefijo_mpi.frecuencia);
            }
        }
    }
    
    prefijos_enviados = num_prefijos_local;
}

void CachePrefijos::mostrarEstadisticas() const {
    if (rank == 0) {
        std::cout << "\n=== ESTADÍSTICAS DE CACHE (Proceso " << rank << ") ===\n";
        std::cout << "Cache hits: " << hits_cache << "\n";
        std::cout << "Cache misses: " << misses_cache << "\n";
        
        if (hits_cache + misses_cache > 0) {
            double tasa_hit = (double)hits_cache / (hits_cache + misses_cache) * 100.0;
            std::cout << "Tasa de aciertos: " << std::fixed << std::setprecision(1) 
                      << tasa_hit << "%\n";
        }
        
        std::cout << "Tamaño del cache: " << cache_local.size() << " entradas\n";
        std::cout << "Prefijos enviados: " << prefijos_enviados << "\n";
        std::cout << "Prefijos recibidos: " << prefijos_recibidos << "\n";
        
        // Mostrar estadísticas agregadas de todos los procesos
        std::vector<int> todos_hits(size), todos_misses(size), todos_tamaños(size);
        
        MPI_Gather(&hits_cache, 1, MPI_INT, todos_hits.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Gather(&misses_cache, 1, MPI_INT, todos_misses.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        int tamaño_local = cache_local.size();
        MPI_Gather(&tamaño_local, 1, MPI_INT, todos_tamaños.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        int total_hits = 0, total_misses = 0, total_entradas = 0;
        for (int i = 0; i < size; i++) {
            total_hits += todos_hits[i];
            total_misses += todos_misses[i];
            total_entradas += todos_tamaños[i];
        }
        
        std::cout << "\n=== ESTADÍSTICAS GLOBALES DE CACHE ===\n";
        std::cout << "Total hits (todos los procesos): " << total_hits << "\n";
        std::cout << "Total misses (todos los procesos): " << total_misses << "\n";
        std::cout << "Total entradas en cache: " << total_entradas << "\n";
        
        if (total_hits + total_misses > 0) {
            double tasa_global = (double)total_hits / (total_hits + total_misses) * 100.0;
            std::cout << "Tasa de aciertos global: " << std::fixed << std::setprecision(1) 
                      << tasa_global << "%\n";
        }
    } else {
        // Procesos no-cero solo envían sus estadísticas
        MPI_Gather(&hits_cache, 1, MPI_INT, nullptr, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Gather(&misses_cache, 1, MPI_INT, nullptr, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        int tamaño_local = cache_local.size();
        MPI_Gather(&tamaño_local, 1, MPI_INT, nullptr, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
}
