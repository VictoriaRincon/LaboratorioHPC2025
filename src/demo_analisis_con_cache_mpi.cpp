#include "../include/calculador_costos.hpp"
#include "../include/escenario.hpp"
#include "../include/cache_prefijos_mpi.hpp"
#include <algorithm>
#include <bitset>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mpi.h>
#include <sstream>
#include <vector>

// Función para verificar si un estado está "prendido" (ON)
bool estaPrendido(EstadoMaquina estado) {
  return estado == EstadoMaquina::ON_CALIENTE ||
         estado == EstadoMaquina::ON_TIBIO || estado == EstadoMaquina::ON_FRIO;
}

// Función para generar la cadena de transiciones prender/apagar
std::string generarTransiciones(const std::vector<EstadoMaquina> &estados) {
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
    return ""; // Nunca se prendió
  }

  std::stringstream ss;
  for (size_t i = 0; i < transiciones.size(); i++) {
    if (i > 0)
      ss << "-";
    ss << transiciones[i];
  }

  return ss.str();
}

// Función para resolver con cache de prefijos
Solucion resolverConCache(const Escenario& escenario, CalculadorCostos& calculador, 
                         CachePrefijos& cache, int max_horas_prefijo = 12) {
    
    // Intentar usar cache para prefijos
    std::vector<EstadoMaquina> mejor_solucion_parcial;
    double mejor_costo_parcial = std::numeric_limits<double>::infinity();
    bool cache_utilizado = false;
    
    // Probar diferentes longitudes de prefijo (de mayor a menor)
    for (int len_prefijo = max_horas_prefijo; len_prefijo >= 4; len_prefijo--) {
        // Crear patrón del escenario para las primeras len_prefijo horas
        std::string patron_prefijo;
        for (int hora = 0; hora < len_prefijo; hora++) {
            double energia_otras = escenario.getEnergiaOtrasFuentes(hora);
            patron_prefijo += (energia_otras > 0) ? '1' : '0';
        }
        
        EntradaCache entrada_cache;
        if (cache.buscarPrefijo(patron_prefijo, entrada_cache)) {
            // Cache hit! Usar esta solución parcial
            mejor_solucion_parcial = entrada_cache.estados_parciales;
            mejor_costo_parcial = entrada_cache.costo_acumulado;
            cache_utilizado = true;
            break;
        }
    }
    
    // Resolver desde donde terminó el cache (o desde el inicio)
    auto inicio_computo = std::chrono::steady_clock::now();
    
    Solucion solucion;
    if (cache_utilizado) {
        // Crear un escenario parcial para las horas restantes
        Escenario escenario_restante;
        int horas_restantes = 24 - mejor_solucion_parcial.size();
        
        for (int i = 0; i < horas_restantes; i++) {
            int hora_global = mejor_solucion_parcial.size() + i;
            escenario_restante.setDemanda(i, escenario.getDemanda(hora_global));
            escenario_restante.setEnergiaOtrasFuentes(i, escenario.getEnergiaOtrasFuentes(hora_global));
        }
        
        // Resolver solo la parte restante
        CalculadorCostos calculador_restante(escenario_restante);
        calculador_restante.configurarCostos(1.0, 2.5, 5.0);
        
        // Silenciar salida
        std::streambuf *old_cout = std::cout.rdbuf();
        std::cout.rdbuf(nullptr);
        Solucion solucion_restante = calculador_restante.resolver();
        std::cout.rdbuf(old_cout);
        
        // Combinar soluciones
        solucion.estados_por_hora = mejor_solucion_parcial;
        solucion.estados_por_hora.insert(solucion.estados_por_hora.end(), 
                                        solucion_restante.estados_por_hora.begin(),
                                        solucion_restante.estados_por_hora.end());
        solucion.costo_total = mejor_costo_parcial + solucion_restante.costo_total;
        solucion.es_valida = solucion_restante.es_valida;
        
    } else {
        // Resolver completo (sin cache)
        std::streambuf *old_cout = std::cout.rdbuf();
        std::cout.rdbuf(nullptr);
        solucion = calculador.resolver();
        std::cout.rdbuf(old_cout);
    }
    
    auto fin_computo = std::chrono::steady_clock::now();
    double tiempo_computo = std::chrono::duration<double, std::milli>(fin_computo - inicio_computo).count();
    
    // Si no usamos cache y la solución es válida, considerar agregar prefijos al cache
    if (!cache_utilizado && solucion.es_valida) {
        // Agregar diferentes longitudes de prefijo al cache
        for (int len = 4; len <= std::min(12, (int)solucion.estados_por_hora.size()); len++) {
            std::string patron_para_cache;
            for (int hora = 0; hora < len; hora++) {
                double energia_otras = escenario.getEnergiaOtrasFuentes(hora);
                patron_para_cache += (energia_otras > 0) ? '1' : '0';
            }
            
            std::vector<EstadoMaquina> estados_prefijo(solucion.estados_por_hora.begin(), 
                                                     solucion.estados_por_hora.begin() + len);
            
            // Calcular costo parcial (aproximado)
            double costo_parcial = solucion.costo_total * ((double)len / 24.0);
            
            cache.agregarPrefijo(patron_para_cache, estados_prefijo, costo_parcial, tiempo_computo);
        }
    }
    
    return solucion;
}

int main(int argc, char *argv[]) {
  // Inicializar MPI
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // Inicializar cache de prefijos
  CachePrefijos cache(rank, size);
  cache.configurar(0.001, 1, 5000);  // umbral_tiempo=0.001ms, min_freq=1, max_size=5000
  cache.habilitarLogging(true);      // Habilitar logging de prefijos

  uint32_t num_combinaciones = 0;

  // Solo el proceso 0 lee la entrada
  if (rank == 0) {
    std::cout << "=== ANALIZADOR MASIVO CON CACHE DE PREFIJOS MPI ===\n";
    std::cout << "Procesos MPI: " << size << "\n";
    std::cin >> num_combinaciones;

    if (num_combinaciones > 16777216) {
      num_combinaciones = 16777216;
    }
    std::cout << "Procesando " << num_combinaciones << " combinaciones con "
              << size << " procesos y cache de prefijos...\n";
  }

  // Broadcast del número de combinaciones a todos los procesos
  MPI_Bcast(&num_combinaciones, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);

  // Calcular el rango de combinaciones para cada proceso
  uint32_t combinaciones_por_proceso = num_combinaciones / size;
  uint32_t resto = num_combinaciones % size;

  uint32_t inicio_local =
      rank * combinaciones_por_proceso + std::min((uint32_t)rank, resto);
  uint32_t fin_local =
      inicio_local + combinaciones_por_proceso + (rank < resto ? 1 : 0);
  uint32_t num_locales = fin_local - inicio_local;

  // Abrir archivo temporal inmediatamente para escribir resultados
  std::string archivo_temp = "resultados_temp_" + std::to_string(rank) + ".csv";
  std::ofstream archivo_local(archivo_temp);
  
  if (!archivo_local.is_open()) {
    std::cerr << "Error: No se pudo crear archivo temporal: " << archivo_temp << std::endl;
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  // Demanda fija según especificación del usuario
  std::vector<double> demanda_fija = {
      300,  200, 100, 100, 100, 200,  300,  500,  800, 1000, 1000, 1000,
      1000, 900, 800, 800, 800, 1000, 1000, 1000, 600, 600,  400,  300};

  auto inicio = std::chrono::steady_clock::now();

  // Variables locales para estadísticas
  double mejor_costo_local = std::numeric_limits<double>::infinity();
  uint32_t combinacion_optima_local = 0;
  uint32_t soluciones_validas_local = 0;
  double suma_costos_local = 0.0;

  // Procesar las combinaciones asignadas a este proceso
  const uint32_t INTERVALO_REPORTE = std::max(100U, num_locales / 10);
  const uint32_t INTERVALO_INTERCAMBIO = std::max(1000U, num_locales / 5);  // Intercambiar cache cada 20%

  for (uint32_t i = 0; i < num_locales; i++) {
    uint32_t combinacion = inicio_local + i;

    // Convertir número a patrón binario
    std::bitset<24> patron_eolica(combinacion);

    // Configurar escenario (SIN salida de debug)
    Escenario escenario;
    for (int hora = 0; hora < 24; hora++) {
      escenario.setDemanda(hora, demanda_fija[hora]);
      double energia_eolica = patron_eolica[23 - hora] ? 500.0 : 0.0;
      escenario.setEnergiaOtrasFuentes(hora, energia_eolica);
    }

    // Resolver usando cache de prefijos
    CalculadorCostos calculador(escenario);
    calculador.configurarCostos(1.0, 2.5, 5.0);
    
    Solucion solucion = resolverConCache(escenario, calculador, cache);

    // Contar horas críticas
    int horas_criticas = 0;
    for (int hora = 0; hora < 24; hora++) {
      if (!escenario.demandaCubiertaConEO(hora)) {
        horas_criticas++;
      }
    }

    // Generar cadena de transiciones
    std::string transiciones = "";
    if (solucion.es_valida) {
      transiciones = generarTransiciones(solucion.estados_por_hora);
    }

    // Escribir resultado directamente al archivo temporal
    archivo_local << combinacion << ","
                  << patron_eolica.to_string() << ","
                  << std::fixed << std::setprecision(2) << solucion.costo_total << ","
                  << (solucion.es_valida ? "SI" : "NO") << ","
                  << horas_criticas << ","
                  << transiciones << "\n";

    // Actualizar estadísticas locales
    if (solucion.es_valida) {
      soluciones_validas_local++;
      suma_costos_local += solucion.costo_total;
      if (solucion.costo_total < mejor_costo_local) {
        mejor_costo_local = solucion.costo_total;
        combinacion_optima_local = combinacion;
      }
    }

    // Intercambio periódico de cache entre procesos
    if ((i + 1) % INTERVALO_INTERCAMBIO == 0) {
      cache.intercambiarPrefijosValiosos();
    }

    // Mostrar progreso solo en proceso 0
    if (rank == 0 &&
        ((i + 1) % INTERVALO_REPORTE == 0 || i + 1 == num_locales)) {
      auto ahora = std::chrono::steady_clock::now();
      auto duracion =
          std::chrono::duration_cast<std::chrono::seconds>(ahora - inicio);

      double porcentaje = (double)(i + 1) / num_locales * 100.0;
      double tasa = (double)(i + 1) / std::max(1, (int)duracion.count());

      std::cout << "\rProceso 0 - Progreso: " << std::fixed
                << std::setprecision(1) << porcentaje << "% | "
                << "Casos: " << (i + 1) << "/" << num_locales << " | "
                << "Tasa: " << std::fixed << std::setprecision(0) << tasa
                << " c/s | Cache: " << cache.getHits() << "H/" << cache.getMisses() << "M"
                << std::flush;
    }
  }

  // Cerrar archivo temporal
  archivo_local.close();

  // Intercambio final de cache
  cache.intercambiarPrefijosValiosos();

  // Sincronizar todos los procesos
  MPI_Barrier(MPI_COMM_WORLD);

  // Solo el proceso 0 combina todos los archivos
  if (rank == 0) {
    std::ofstream archivo_final("resultados_demo_cache.csv");
    archivo_final << "CombinacionID,PatronEolica,CostoTotal,SolucionValida,HorasCriticas,Transiciones\n";
    
    // Leer y combinar todos los archivos temporales EN ORDEN
    for (int r = 0; r < size; r++) {
      std::string archivo_temp_r = "resultados_temp_" + std::to_string(r) + ".csv";
      std::ifstream archivo_temp_read(archivo_temp_r);
      
      if (archivo_temp_read.is_open()) {
        std::string linea;
        while (std::getline(archivo_temp_read, linea)) {
          archivo_final << linea << "\n";
        }
        archivo_temp_read.close();
        
        // Eliminar archivo temporal
        std::remove(archivo_temp_r.c_str());
      } else {
        std::cerr << "Advertencia: No se pudo leer archivo temporal: " << archivo_temp_r << std::endl;
      }
    }
    archivo_final.close();
  }

  // Recopilar estadísticas globales
  uint32_t soluciones_validas_global;
  double suma_costos_global;
  double mejor_costo_global;
  uint32_t combinacion_optima_global;

  MPI_Reduce(&soluciones_validas_local, &soluciones_validas_global, 1,
             MPI_UINT32_T, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&suma_costos_local, &suma_costos_global, 1, MPI_DOUBLE, MPI_SUM, 0,
             MPI_COMM_WORLD);
  MPI_Reduce(&mejor_costo_local, &mejor_costo_global, 1, MPI_DOUBLE, MPI_MIN, 0,
             MPI_COMM_WORLD);

  // Para encontrar la combinación óptima global
  struct {
    double costo;
    int rank;
  } local_min, global_min;

  local_min.costo = mejor_costo_local;
  local_min.rank = rank;

  MPI_Reduce(&local_min, &global_min, 1, MPI_DOUBLE_INT, MPI_MINLOC, 0,
             MPI_COMM_WORLD);

  // El proceso que tiene el mínimo envía su combinación óptima
  if (rank == global_min.rank) {
    MPI_Send(&combinacion_optima_local, 1, MPI_UINT32_T, 0, 0, MPI_COMM_WORLD);
  }

  if (rank == 0 && global_min.rank != 0) {
    MPI_Recv(&combinacion_optima_global, 1, MPI_UINT32_T, global_min.rank, 0,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  } else if (rank == 0) {
    combinacion_optima_global = combinacion_optima_local;
  }

  // Solo el proceso 0 muestra los resultados finales
  if (rank == 0) {
    auto fin = std::chrono::steady_clock::now();
    auto tiempo_total =
        std::chrono::duration_cast<std::chrono::seconds>(fin - inicio);

    std::cout << "\n\n=== PROCESAMIENTO COMPLETADO ===\n";
    std::cout << "Combinaciones procesadas: " << num_combinaciones << "\n";
    std::cout << "Procesos MPI utilizados: " << size << "\n";
    std::cout << "Tiempo total: " << tiempo_total.count() << " segundos\n";
    std::cout << "Tasa promedio: " << std::fixed << std::setprecision(1)
              << (double)num_combinaciones / tiempo_total.count()
              << " combinaciones/segundo\n";
    std::cout << "Soluciones válidas: " << soluciones_validas_global << " ("
              << std::fixed << std::setprecision(1)
              << (double)soluciones_validas_global / num_combinaciones * 100.0
              << "%)\n";
    std::cout << "Mejor costo encontrado: " << mejor_costo_global
              << " (combinación #" << combinacion_optima_global << ")\n";

    if (soluciones_validas_global > 0) {
      double costo_promedio = suma_costos_global / soluciones_validas_global;
      std::cout << "Costo promedio: " << std::fixed << std::setprecision(2)
                << costo_promedio << "\n";
    }
    
    std::cout << "Resultados con cache guardados en: resultados_demo_cache.csv\n";
  }

  // Mostrar estadísticas del cache
  cache.mostrarEstadisticas();

  // Cerrar archivos de log
  cache.cerrarLog();

  MPI_Finalize();
  return 0;
}
