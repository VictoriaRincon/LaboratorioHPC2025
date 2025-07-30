#include "../include/interfaz_terminal.hpp"
#ifdef _OPENMP_DISABLED
    // Definir macros vacías para compatibilidad
    #define omp_get_max_threads() 1
    #define omp_get_thread_num() 0
    #define omp_set_num_threads(n) 
#else
    #ifdef _OPENMP
        #include <omp.h>
    #else
        // OpenMP no disponible
        #define omp_get_max_threads() 1
        #define omp_get_thread_num() 0
        #define omp_set_num_threads(n)
    #endif
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdlib>

void InterfazTerminal::ejecutar() {
    imprimirTitulo("SISTEMA DE OPTIMIZACIÓN DE MÁQUINA DE ESTADOS");
    std::cout << "Implementación de algoritmo de backtracking con memoización" << std::endl;
    std::cout << "Versión: Secuencial (preparada para paralelización MPI)" << std::endl;
    imprimirSeparador();
    
    int opcion;
    do {
        mostrarMenu();
        opcion = leerEntero("Seleccione una opción", 0, 7);
        
        imprimirSeparador('-');
        
        switch (opcion) {
            case 1:
                cargarArchivoParametros();
                break;
            case 2:
                ingresarParametrosManual();
                break;
            case 3:
                ejecutarPruebaRapida();
                break;
            case 4:
                ejecutarPruebasValidacion();
                break;
            case 5:
                ejecutarBenchmarkRendimiento();
                break;
            case 6:
                ejecutarBenchmarkEscalabilidad();
                break;
            case 7:
                mostrarAyuda();
                break;
            case 0:
                std::cout << "¡Gracias por usar el sistema! 👋" << std::endl;
                break;
            default:
                std::cout << "❌ Opción no válida" << std::endl;
        }
        
        if (opcion != 0) {
            std::cout << "\nPresione Enter para continuar...";
            std::cin.ignore();
            std::cin.get();
        }
        
    } while (opcion != 0);
}

void InterfazTerminal::mostrarMenu() {
    imprimirSeparador();
    std::cout << "MENÚ PRINCIPAL" << std::endl;
    std::cout << "1. 📁 Cargar parámetros desde archivo (data/parametros.in)" << std::endl;
    std::cout << "2. ⌨️  Ingresar parámetros manualmente" << std::endl;
    std::cout << "3. ⚡ Ejecutar prueba rápida" << std::endl;
    std::cout << "4. 🧪 Ejecutar batería de pruebas de validación" << std::endl;
    std::cout << "5. 📊 Benchmark de rendimiento (todas las combinaciones)" << std::endl;
    std::cout << "6. 📈 Benchmark de escalabilidad (múltiples largos)" << std::endl;
    std::cout << "7. ❓ Mostrar ayuda" << std::endl;
    std::cout << "0. 🚪 Salir" << std::endl;
    imprimirSeparador();
}

void InterfazTerminal::procesarArgumentos(int argc, char* argv[]) {
    if (argc == 1) {
        ejecutar();
        return;
    }
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            mostrarAyuda();
            return;
        } else if (arg == "--test" || arg == "-t") {
            ejecutarPruebasValidacion();
            return;
        } else if (arg == "--quick" || arg == "-q") {
            ejecutarPruebaRapida();
            return;
        } else if (arg == "--file" || arg == "-f") {
            cargarArchivoParametros();
            return;
        } else if (arg == "--benchmark" || arg == "-b") {
            ejecutarBenchmarkRendimiento();
            return;
        } else {
            std::cout << "❌ Argumento desconocido: " << arg << std::endl;
            std::cout << "Use --help para ver las opciones disponibles" << std::endl;
            return;
        }
    }
}

void InterfazTerminal::cargarArchivoParametros() {
    imprimirTitulo("CARGAR DESDE ARCHIVO");
    
    std::string archivo = "data/parametros.in";
    std::cout << "Cargando desde: " << archivo << std::endl;
    
    try {
        auto parametros = leerArchivoParametros(archivo);
        
        if (parametros.empty()) {
            std::cout << "❌ Error: Archivo vacío o sin datos válidos" << std::endl;
            return;
        }
        
        std::cout << "✅ Parámetros cargados exitosamente" << std::endl;
        std::cout << "Datos: ";
        for (size_t i = 0; i < parametros.size(); i++) {
            std::cout << parametros[i];
            if (i < parametros.size() - 1) std::cout << " ";
        }
        std::cout << std::endl;
        
        mostrarEstadisticasDetalladas(parametros);
        
        if (confirmar("¿Desea resolver este problema?")) {
            auto inicio = std::chrono::high_resolution_clock::now();
            
            OptimizadorMaquina optimizador(parametros);
            auto solucion = optimizador.resolver();
            
            auto fin = std::chrono::high_resolution_clock::now();
            auto duracion = std::chrono::duration_cast<std::chrono::microseconds>(fin - inicio);
            
            mostrarSolucion(solucion, parametros);
            std::cout << "⏱️  Tiempo de ejecución: " << formatearTiempo(duracion.count() / 1000000.0) << std::endl;
            
            validarSolucion(solucion, parametros);
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error al cargar archivo: " << e.what() << std::endl;
    }
}

void InterfazTerminal::ingresarParametrosManual() {
    imprimirTitulo("ENTRADA MANUAL DE PARÁMETROS");
    
    std::cout << "Ingrese la secuencia de disponibilidad de energía eólica" << std::endl;
    std::cout << "Formato: valores separados por espacios (ej: 1 1 0 0 1)" << std::endl;
    std::cout << "Donde 0 = requiere generación, 1 = energía eólica suficiente" << std::endl;
    
    std::string entrada = leerEntrada("Parámetros");
    
    try {
        auto parametros = parsearParametrosString(entrada);
        
        if (parametros.empty()) {
            std::cout << "❌ Error: No se pudieron interpretar los parámetros" << std::endl;
            return;
        }
        
        std::cout << "✅ Parámetros interpretados correctamente" << std::endl;
        mostrarEstadisticasDetalladas(parametros);
        
        if (confirmar("¿Desea resolver este problema?")) {
            auto inicio = std::chrono::high_resolution_clock::now();
            
            OptimizadorMaquina optimizador(parametros);
            auto solucion = optimizador.resolver();
            
            auto fin = std::chrono::high_resolution_clock::now();
            auto duracion = std::chrono::duration_cast<std::chrono::microseconds>(fin - inicio);
            
            mostrarSolucion(solucion, parametros);
            std::cout << "⏱️  Tiempo de ejecución: " << formatearTiempo(duracion.count() / 1000000.0) << std::endl;
            
            validarSolucion(solucion, parametros);
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error al procesar parámetros: " << e.what() << std::endl;
    }
}

void InterfazTerminal::ejecutarPruebaRapida() {
    imprimirTitulo("PRUEBA RÁPIDA");
    
    std::cout << "Ejecutando caso de prueba del archivo parametros.in..." << std::endl;
    
    try {
        auto parametros = leerArchivoParametros("data/parametros.in");
        
        if (parametros.empty()) {
            std::cout << "⚠️  Archivo no encontrado, usando caso de prueba por defecto..." << std::endl;
            parametros = {1, 1, 1, 0, 0, 1, 1, 0};
        }
        
        auto inicio = std::chrono::high_resolution_clock::now();
        
        OptimizadorMaquina optimizador(parametros);
        auto solucion = optimizador.resolver();
        
        auto fin = std::chrono::high_resolution_clock::now();
        auto duracion = std::chrono::duration_cast<std::chrono::microseconds>(fin - inicio);
        
        mostrarSolucion(solucion, parametros);
        std::cout << "⏱️  Tiempo de ejecución: " << formatearTiempo(duracion.count() / 1000000.0) << std::endl;
        
        validarSolucion(solucion, parametros);
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error en prueba rápida: " << e.what() << std::endl;
    }
}

void InterfazTerminal::ejecutarPruebasValidacion() {
    imprimirTitulo("BATERÍA DE PRUEBAS DE VALIDACIÓN");
    
    std::cout << "Ejecutando múltiples casos de prueba para validar correctitud..." << std::endl;
    imprimirSeparador('-');
    
    ejecutarPruebaCritica();
    ejecutarPruebaOptima();
    ejecutarPruebaMixta();
    ejecutarPruebaCompleja();
    
    std::cout << "✅ Batería de pruebas completada" << std::endl;
}

void InterfazTerminal::ejecutarBenchmarkRendimiento() {
    imprimirTitulo("BENCHMARK DE RENDIMIENTO - TODAS LAS COMBINACIONES");
    
    std::cout << "Este benchmark genera y resuelve TODAS las combinaciones posibles" << std::endl;
    std::cout << "de un largo específico para medir el rendimiento del algoritmo." << std::endl;
    std::cout << std::endl;
    
    std::cout << "📊 Largos recomendados por rendimiento:" << std::endl;
    std::cout << "• 1-8 bits:  Instantáneo (hasta 256 combinaciones)" << std::endl;
    std::cout << "• 9-12 bits: Rápido (hasta 4,096 combinaciones)" << std::endl;
    std::cout << "• 13-16 bits: Moderado (hasta 65,536 combinaciones)" << std::endl;
    std::cout << "• 17-20 bits: Considerable (hasta 1,048,576 combinaciones)" << std::endl;
    std::cout << "• 21-25 bits: Extremo (hasta 33,554,432 combinaciones)" << std::endl;
    std::cout << "• 26-30 bits: MASIVO (hasta 1,073,741,824 combinaciones) - ¡Para análisis MPI!" << std::endl;
    std::cout << std::endl;
    
    int largo = leerEntero("Ingrese el largo del problema (bits)", 1, 30);
    
    if (largo > 16) {
        long long combinaciones = 1LL << largo;
        std::cout << "⚠️  ADVERTENCIA: " << combinaciones << " combinaciones a procesar." << std::endl;
        
        if (largo > 20) {
            std::cout << "🚨 ATENCIÓN: Más de 20 bits puede tomar HORAS o DÍAS." << std::endl;
            std::cout << "💡 Recomendamos usar MPI para estos casos." << std::endl;
        }
        if (largo > 25) {
            std::cout << "🔥 EXTREMO: Más de 25 bits puede requerir SEMANAS." << std::endl;
            std::cout << "⚡ Este caso está diseñado para análisis de escalabilidad MPI." << std::endl;
        }
        
        std::cout << "¿Desea continuar? (s/n): ";
        char respuesta;
        std::cin >> respuesta;
        if (respuesta != 's' && respuesta != 'S') {
            std::cout << "Benchmark cancelado." << std::endl;
            return;
        }
    }
    
    bool mostrar_progreso = confirmar("¿Mostrar barra de progreso?");
    bool exportar_csv = confirmar("¿Exportar resultados a CSV?");
    
    // Configuración de paralelización OpenMP
    std::cout << std::endl;
    std::cout << "🔄 CONFIGURACIÓN DE PARALELIZACIÓN:" << std::endl;
    std::cout << "Núcleos disponibles en el sistema: " << std::endl;
    
    // Obtener información del sistema
    system("sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 'No disponible'");
    
    int max_hilos = 1;
    #if defined(_OPENMP) && !defined(_OPENMP_DISABLED)
    max_hilos = omp_get_max_threads();
    std::cout << "Hilos máximos OpenMP: " << max_hilos << std::endl;
    #else
    std::cout << "OpenMP no disponible - usando modo secuencial" << std::endl;
    #endif
    
    int num_hilos = leerEntero("¿Cuántos hilos usar para paralelización?", 1, max_hilos);
    if (num_hilos == 1) {
        std::cout << "🔄 Modo secuencial seleccionado" << std::endl;
    } else {
        std::cout << "🔄 Paralelización OpenMP con " << num_hilos << " hilos" << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "🚀 Iniciando benchmark..." << std::endl;
    
    try {
        auto inicio_benchmark = std::chrono::high_resolution_clock::now();
        
        BenchmarkSistema benchmark;
        benchmark.configurarVerbosidad(true);
        benchmark.configurarHilos(num_hilos);
        
        auto resultado = benchmark.ejecutarBenchmark(largo, mostrar_progreso);
        
        auto fin_benchmark = std::chrono::high_resolution_clock::now();
        auto duracion_total = std::chrono::duration_cast<std::chrono::milliseconds>(fin_benchmark - inicio_benchmark);
        
        std::cout << std::endl;
        benchmark.mostrarResultados(resultado);
        
        std::cout << "⏱️  Tiempo total de benchmark (incluyendo overhead): " 
                  << formatearTiempo(duracion_total.count()) << std::endl;
        
        if (exportar_csv) {
            std::string archivo = "resultados/benchmark_" + std::to_string(largo) + "_bits.csv";
            benchmark.exportarCSV(resultado, archivo);
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error durante el benchmark: " << e.what() << std::endl;
    }
}

void InterfazTerminal::ejecutarBenchmarkEscalabilidad() {
    imprimirTitulo("BENCHMARK DE ESCALABILIDAD - ANÁLISIS COMPARATIVO");
    
    std::cout << "Este benchmark ejecuta pruebas para múltiples largos" << std::endl;
    std::cout << "y analiza cómo escala el rendimiento del algoritmo." << std::endl;
    std::cout << std::endl;
    
    int largo_minimo = leerEntero("Largo mínimo", 1, 25);
    int largo_maximo = leerEntero("Largo máximo", largo_minimo, 30);
    
    if (largo_maximo > 15) {
        std::cout << "⚠️  ADVERTENCIA: Largos mayores a 15 pueden tomar tiempo considerable." << std::endl;
        if (largo_maximo > 20) {
            std::cout << "🚨 ATENCIÓN: Largos mayores a 20 pueden tomar HORAS." << std::endl;
            std::cout << "💡 Considere usar límites de tiempo o MPI para casos extremos." << std::endl;
        }
        if (!confirmar("¿Desea continuar?")) {
            std::cout << "Benchmark cancelado." << std::endl;
            return;
        }
    }
    
    bool exportar_resultados = confirmar("¿Exportar resultados completos?");
    
    std::cout << std::endl;
    std::cout << "🚀 Iniciando benchmark de escalabilidad..." << std::endl;
    std::cout << "Rango: " << largo_minimo << " a " << largo_maximo << " bits" << std::endl;
    std::cout << std::endl;
    
    try {
        BenchmarkSistema benchmark;
        benchmark.configurarVerbosidad(false);
        
        // Configurar paralelización para escalabilidad
        int num_hilos_escalabilidad = 1;
        #if defined(_OPENMP) && !defined(_OPENMP_DISABLED)
        num_hilos_escalabilidad = omp_get_max_threads();
        std::cout << "🔄 Usando " << num_hilos_escalabilidad << " hilos para análisis de escalabilidad" << std::endl;
        #endif
        benchmark.configurarHilos(num_hilos_escalabilidad);
        
        std::vector<ResultadoBenchmark> resultados;
        
        for (int largo = largo_minimo; largo <= largo_maximo; largo++) {
            std::cout << "📊 Procesando largo " << largo << " bits..." << std::flush;
            
            auto inicio = std::chrono::high_resolution_clock::now();
            auto resultado = benchmark.ejecutarBenchmark(largo, false);
            auto fin = std::chrono::high_resolution_clock::now();
            
            auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(fin - inicio);
            
            resultados.push_back(resultado);
            
            std::cout << " ✅ " << resultado.total_combinaciones << " combinaciones en " 
                      << formatearTiempo(duracion.count()) << std::endl;
        }
        
        std::cout << std::endl;
        benchmark.mostrarAnalisisEscalabilidad(resultados);
        
        if (exportar_resultados) {
            std::string archivo_base = "resultados/escalabilidad_" + 
                                     std::to_string(largo_minimo) + "_" + 
                                     std::to_string(largo_maximo);
            
            for (const auto& resultado : resultados) {
                std::string archivo = archivo_base + "_" + std::to_string(resultado.largo_problema) + ".csv";
                benchmark.exportarCSV(resultado, archivo);
            }
            
            std::cout << "📁 Resultados exportados a directorio 'resultados/'" << std::endl;
        }
        
        // Predicción de escalabilidad
        if (resultados.size() >= 3 && largo_maximo < 25) {
            std::cout << std::endl;
            int largo_prediccion = leerEntero("¿Predecir rendimiento para qué largo?", largo_maximo + 1, 30);
            
            // Predicción simple basada en crecimiento exponencial
            double factor_promedio = 0.0;
            for (size_t i = 1; i < resultados.size(); i++) {
                factor_promedio += resultados[i].tiempo_total_ms / resultados[i-1].tiempo_total_ms;
            }
            factor_promedio /= (resultados.size() - 1);
            
            double tiempo_predicho = resultados.back().tiempo_total_ms;
            for (int i = largo_maximo + 1; i <= largo_prediccion; i++) {
                tiempo_predicho *= factor_promedio;
            }
            
            long long combinaciones_predichas = 1LL << largo_prediccion;
            
            std::cout << std::endl;
            std::cout << "🔮 PREDICCIÓN PARA " << largo_prediccion << " BITS:" << std::endl;
            std::cout << "Combinaciones: " << combinaciones_predichas << std::endl;
            std::cout << "Tiempo estimado: " << formatearTiempo(tiempo_predicho) << std::endl;
            
            if (tiempo_predicho > 60000) { // Más de 1 minuto
                std::cout << "⚠️  Tiempo considerable - considere paralelización MPI" << std::endl;
            }
            if (tiempo_predicho > 3600000) { // Más de 1 hora
                std::cout << "🚨 Tiempo extremo (>1 hora) - MPI es OBLIGATORIO para este caso" << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error durante el benchmark: " << e.what() << std::endl;
    }
}

void InterfazTerminal::mostrarAyuda() {
    imprimirTitulo("AYUDA - SISTEMA DE OPTIMIZACIÓN");
    
    std::cout << "DESCRIPCIÓN:" << std::endl;
    std::cout << "Este sistema optimiza los estados de una máquina de calentamiento" << std::endl;
    std::cout << "para minimizar costos considerando disponibilidad de energía eólica." << std::endl;
    std::cout << std::endl;
    
    std::cout << "ESTADOS DE LA MÁQUINA:" << std::endl;
    std::cout << "• ON/CALIENTE  (5.0) - Genera energía" << std::endl;
    std::cout << "• ON/TIBIO     (2.5) - No genera energía" << std::endl;
    std::cout << "• ON/FRIO      (1.0) - No genera energía" << std::endl;
    std::cout << "• OFF/CALIENTE (0.0) - No genera energía" << std::endl;
    std::cout << "• OFF/TIBIO    (0.0) - No genera energía" << std::endl;
    std::cout << "• OFF/FRIO     (0.0) - No genera energía" << std::endl;
    std::cout << std::endl;
    
    std::cout << "FORMATO DE ENTRADA:" << std::endl;
    std::cout << "• 0 = Energía eólica insuficiente (requiere ON/CALIENTE)" << std::endl;
    std::cout << "• 1 = Energía eólica suficiente (cualquier estado válido)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "ARGUMENTOS DE LÍNEA DE COMANDOS:" << std::endl;
    std::cout << "• --help, -h    : Mostrar esta ayuda" << std::endl;
    std::cout << "• --test, -t    : Ejecutar pruebas de validación" << std::endl;
    std::cout << "• --quick, -q   : Ejecutar prueba rápida" << std::endl;
    std::cout << "• --file, -f    : Cargar desde archivo" << std::endl;
    std::cout << "• --benchmark, -b : Ejecutar benchmark de rendimiento" << std::endl;
    std::cout << std::endl;
    
    std::cout << "ALGORITMO:" << std::endl;
    std::cout << "• Programación dinámica con memoización" << std::endl;
    std::cout << "• Complejidad: O(H × S²) donde H=horas, S=6 estados" << std::endl;
    std::cout << "• Garantiza optimalidad global" << std::endl;
}

// Implementaciones de funciones auxiliares
std::vector<int> InterfazTerminal::leerArchivoParametros(const std::string& archivo) {
    std::ifstream file(archivo);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + archivo);
    }
    
    std::vector<int> parametros;
    std::string linea;
    
    while (std::getline(file, linea)) {
        if (linea.empty() || linea[0] == '#') continue;
        
        std::istringstream iss(linea);
        int valor;
        while (iss >> valor) {
            if (valor == 0 || valor == 1) {
                parametros.push_back(valor);
            } else {
                throw std::runtime_error("Valor inválido en archivo: " + std::to_string(valor));
            }
        }
        
        if (!parametros.empty()) break; // Solo leer la primera línea con datos
    }
    
    return parametros;
}

std::vector<int> InterfazTerminal::parsearParametrosString(const std::string& entrada) {
    std::vector<int> parametros;
    std::istringstream iss(entrada);
    int valor;
    
    while (iss >> valor) {
        if (valor == 0 || valor == 1) {
            parametros.push_back(valor);
        } else {
            throw std::runtime_error("Valor inválido: " + std::to_string(valor));
        }
    }
    
    return parametros;
}

void InterfazTerminal::mostrarSolucion(const Solucion& solucion, const std::vector<int>& energia_eolica) {
    imprimirTitulo("SOLUCIÓN ENCONTRADA");
    
    if (!solucion.es_factible) {
        std::cout << "❌ No se encontró solución factible" << std::endl;
        return;
    }
    
    std::cout << "💰 Costo total: " << std::fixed << std::setprecision(1) << solucion.costo_total << std::endl;
    std::cout << "🕐 Horizonte temporal: " << solucion.secuencia_estados.size() << " horas" << std::endl;
    std::cout << std::endl;
    
    std::cout << "SECUENCIA DE ESTADOS:" << std::endl;
    std::cout << std::setw(4) << "Hora" << " | " 
              << std::setw(15) << "Estado" << " | "
              << std::setw(5) << "Costo" << " | "
              << std::setw(6) << "Eólica" << " | "
              << "Req_Gen" << std::endl;
    imprimirSeparador('-', 50);
    
    for (size_t i = 0; i < solucion.secuencia_estados.size(); i++) {
        auto estado = solucion.secuencia_estados[i];
        double costo = GestorEstados::obtenerCosto(estado);
        bool requiere_gen = energia_eolica[i] == 0;
        
        std::cout << std::setw(4) << i << " | "
                  << std::setw(15) << GestorEstados::estadoAString(estado) << " | "
                  << std::setw(5) << std::fixed << std::setprecision(1) << costo << " | "
                  << std::setw(6) << energia_eolica[i] << " | "
                  << (requiere_gen ? "Sí" : "No") << std::endl;
    }
}

void InterfazTerminal::mostrarEstadisticasDetalladas(const std::vector<int>& energia_eolica) {
    int horas_con_eolica = std::count(energia_eolica.begin(), energia_eolica.end(), 1);
    int horas_sin_eolica = energia_eolica.size() - horas_con_eolica;
    
    std::cout << std::endl;
    std::cout << "📊 ANÁLISIS DEL ESCENARIO:" << std::endl;
    std::cout << "Horizonte temporal: " << energia_eolica.size() << " horas" << std::endl;
    std::cout << "Horas con energía eólica: " << horas_con_eolica << "/" << energia_eolica.size() << std::endl;
    std::cout << "Horas que requieren generación: " << horas_sin_eolica << "/" << energia_eolica.size() << std::endl;
    
    if (horas_sin_eolica == 0) {
        std::cout << "✅ ESCENARIO ÓPTIMO: Costo esperado = 0" << std::endl;
    } else if (horas_con_eolica == 0) {
        std::cout << "⚠️  ESCENARIO CRÍTICO: Costo mínimo = " << (horas_sin_eolica * 5.0) << std::endl;
    } else {
        std::cout << "⚖️  ESCENARIO MIXTO: Se requiere optimización" << std::endl;
    }
    std::cout << std::endl;
}

void InterfazTerminal::validarSolucion(const Solucion& solucion, const std::vector<int>& energia_eolica) {
    std::cout << std::endl;
    imprimirTitulo("VALIDACIÓN DE SOLUCIÓN");
    
    if (!solucion.es_factible) {
        std::cout << "❌ Solución no factible - no hay nada que validar" << std::endl;
        return;
    }
    
    bool valida = true;
    std::vector<std::string> errores;
    
    // Verificar que se satisfaga la demanda en cada hora
    for (size_t i = 0; i < energia_eolica.size(); i++) {
        if (energia_eolica[i] == 0) {  // Requiere generación
            if (!GestorEstados::puedeGenerarEnergia(solucion.secuencia_estados[i])) {
                errores.push_back("Hora " + std::to_string(i) + ": Demanda no satisfecha");
                valida = false;
            }
        }
    }
    
    // Verificar transiciones válidas
    for (size_t i = 1; i < solucion.secuencia_estados.size(); i++) {
        auto estado_anterior = solucion.secuencia_estados[i-1];
        auto estado_actual = solucion.secuencia_estados[i];
        
        if (!GestorEstados::esTransicionValida(estado_anterior, estado_actual)) {
            errores.push_back("Transición inválida en hora " + std::to_string(i-1) + " -> " + std::to_string(i));
            valida = false;
        }
    }
    
    // Verificar costo total
    double costo_calculado = 0.0;
    for (auto estado : solucion.secuencia_estados) {
        costo_calculado += GestorEstados::obtenerCosto(estado);
    }
    
    if (std::abs(costo_calculado - solucion.costo_total) > 0.001) {
        errores.push_back("Costo total inconsistente");
        valida = false;
    }
    
    if (valida) {
        std::cout << "✅ Solución VÁLIDA - Todas las restricciones se cumplen" << std::endl;
    } else {
        std::cout << "❌ Solución INVÁLIDA - Se encontraron errores:" << std::endl;
        for (const auto& error : errores) {
            std::cout << "  • " << error << std::endl;
        }
    }
}

// Implementaciones de pruebas predefinidas
void InterfazTerminal::ejecutarPruebaCritica() {
    std::cout << "🔴 PRUEBA CRÍTICA (sin energía eólica):" << std::endl;
    std::vector<int> parametros = {0, 0, 0, 0};
    
    OptimizadorMaquina optimizador(parametros);
    auto solucion = optimizador.resolver();
    
    std::cout << "Entrada: ";
    for (int p : parametros) std::cout << p << " ";
    std::cout << std::endl;
    std::cout << "Costo: " << solucion.costo_total << " (esperado: 20.0)" << std::endl;
    std::cout << (solucion.costo_total == 20.0 ? "✅ CORRECTO" : "❌ ERROR") << std::endl;
    std::cout << std::endl;
}

void InterfazTerminal::ejecutarPruebaOptima() {
    std::cout << "🟢 PRUEBA ÓPTIMA (energía eólica completa):" << std::endl;
    std::vector<int> parametros = {1, 1, 1, 1};
    
    OptimizadorMaquina optimizador(parametros);
    auto solucion = optimizador.resolver();
    
    std::cout << "Entrada: ";
    for (int p : parametros) std::cout << p << " ";
    std::cout << std::endl;
    std::cout << "Costo: " << solucion.costo_total << " (esperado: 0.0)" << std::endl;
    std::cout << (solucion.costo_total == 0.0 ? "✅ CORRECTO" : "❌ ERROR") << std::endl;
    std::cout << std::endl;
}

void InterfazTerminal::ejecutarPruebaMixta() {
    std::cout << "🟡 PRUEBA MIXTA (escenario combinado):" << std::endl;
    std::vector<int> parametros = {1, 0, 1, 0};
    
    OptimizadorMaquina optimizador(parametros);
    auto solucion = optimizador.resolver();
    
    std::cout << "Entrada: ";
    for (int p : parametros) std::cout << p << " ";
    std::cout << std::endl;
    std::cout << "Costo: " << solucion.costo_total << std::endl;
    
    validarSolucion(solucion, parametros);
    std::cout << std::endl;
}

void InterfazTerminal::ejecutarPruebaCompleja() {
    std::cout << "🔵 PRUEBA COMPLEJA (caso de 8 horas):" << std::endl;
    std::vector<int> parametros = {1, 1, 0, 0, 1, 0, 1, 1};
    
    auto inicio = std::chrono::high_resolution_clock::now();
    
    OptimizadorMaquina optimizador(parametros);
    auto solucion = optimizador.resolver();
    
    auto fin = std::chrono::high_resolution_clock::now();
    auto duracion = std::chrono::duration_cast<std::chrono::microseconds>(fin - inicio);
    
    std::cout << "Entrada: ";
    for (int p : parametros) std::cout << p << " ";
    std::cout << std::endl;
    std::cout << "Costo: " << solucion.costo_total << std::endl;
    std::cout << "Tiempo: " << formatearTiempo(duracion.count() / 1000000.0) << std::endl;
    
    validarSolucion(solucion, parametros);
    std::cout << std::endl;
}

// Implementaciones de utilidades
std::string InterfazTerminal::leerEntrada(const std::string& prompt) {
    std::cout << prompt << ": ";
    std::string entrada;
    std::cin.ignore();
    std::getline(std::cin, entrada);
    return entrada;
}

int InterfazTerminal::leerEntero(const std::string& prompt, int min_val, int max_val) {
    int valor;
    do {
        std::cout << prompt << " (" << min_val << "-" << max_val << "): ";
        std::cin >> valor;
        
        if (std::cin.fail() || valor < min_val || valor > max_val) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "❌ Valor inválido. Intente nuevamente." << std::endl;
        } else {
            break;
        }
    } while (true);
    
    return valor;
}

bool InterfazTerminal::confirmar(const std::string& mensaje) {
    std::cout << mensaje << " (s/n): ";
    char respuesta;
    std::cin >> respuesta;
    return respuesta == 's' || respuesta == 'S';
}

void InterfazTerminal::imprimirSeparador(char caracter, int longitud) {
    std::cout << std::string(longitud, caracter) << std::endl;
}

void InterfazTerminal::imprimirTitulo(const std::string& titulo) {
    imprimirSeparador('=');
    std::cout << titulo << std::endl;
    imprimirSeparador('=');
}

std::string InterfazTerminal::formatearTiempo(double segundos) {
    if (segundos < 0.001) {
        return std::to_string(int(segundos * 1000000)) + " μs";
    } else if (segundos < 1.0) {
        return std::to_string(int(segundos * 1000)) + " ms";
    } else {
        return std::to_string(segundos) + " s";
    }
} 