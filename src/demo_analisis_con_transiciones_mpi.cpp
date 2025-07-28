#include "../include/calculador_costos.hpp"
#include "../include/escenario.hpp"
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

int main(int argc, char *argv[]) {
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
    std::cout << "Procesando " << num_combinaciones << " combinaciones con "
              << size << " procesos...\n";
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

    // Resolver (silenciar salida)
    CalculadorCostos calculador(escenario);
    calculador.configurarCostos(1.0, 2.5, 5.0);

    // Capturar stdout temporalmente
    std::streambuf *old_cout = std::cout.rdbuf();
    std::cout.rdbuf(nullptr);
    Solucion solucion = calculador.resolver();
    std::cout.rdbuf(old_cout);

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
                << " c/s" << std::flush;
    }
  }

  // Cerrar archivo temporal
  archivo_local.close();

  // Sincronizar todos los procesos
  MPI_Barrier(MPI_COMM_WORLD);

  // Solo el proceso 0 combina todos los archivos
  if (rank == 0) {
    std::ofstream archivo_final("resultados_demo.csv");
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
    
    std::cout << "Resultados con transiciones guardados en: resultados_demo.csv\n";
  }

  MPI_Finalize();
  return 0;
}