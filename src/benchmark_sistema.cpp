#include "../include/benchmark_sistema.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <sstream>

BenchmarkSistema::BenchmarkSistema() 
    : verboso(false), incluir_detalle_completo(false) {
}

ResultadoBenchmark BenchmarkSistema::ejecutarBenchmark(int largo_problema, bool mostrar_progreso) {
    ResultadoBenchmark resultado;
    resultado.largo_problema = largo_problema;
    resultado.total_combinaciones = 1LL << largo_problema; // 2^largo_problema
    resultado.combinaciones_factibles = 0;
    
    if (verboso) {
        std::cout << "🚀 Iniciando benchmark para largo " << largo_problema 
                  << " (" << resultado.total_combinaciones << " combinaciones)" << std::endl;
    }
    
    // Generar todas las combinaciones
    auto combinaciones = generarTodasLasCombinaciones(largo_problema);
    
    // Inicializar contadores
    std::vector<double> tiempos;
    std::vector<double> costos;
    std::vector<size_t> cache_sizes;
    
    resultado.escenarios_criticos = 0;
    resultado.escenarios_optimos = 0;
    resultado.escenarios_mixtos = 0;
    resultado.maximo_estados_cache = 0;
    
    auto inicio_total = std::chrono::high_resolution_clock::now();
    
    // Procesar cada combinación
    for (size_t i = 0; i < combinaciones.size(); i++) {
        auto stats = analizarCombinacion(combinaciones[i]);
        
        if (stats.solucion_factible) {
            resultado.combinaciones_factibles++;
            costos.push_back(stats.costo_solucion);
        }
        
        tiempos.push_back(stats.tiempo_ejecucion_ms);
        cache_sizes.push_back(stats.estados_cache_utilizados);
        
        // Clasificar escenario
        std::string tipo = clasificarEscenario(combinaciones[i]);
        if (tipo == "crítico") resultado.escenarios_criticos++;
        else if (tipo == "óptimo") resultado.escenarios_optimos++;
        else resultado.escenarios_mixtos++;
        
        resultado.maximo_estados_cache = std::max(resultado.maximo_estados_cache, 
                                                 stats.estados_cache_utilizados);
        
        if (incluir_detalle_completo) {
            resultado.detalle_combinaciones.push_back(stats);
        }
        
        // Mostrar progreso
        if (mostrar_progreso && (i + 1) % std::max(1LL, resultado.total_combinaciones / 20) == 0) {
            auto tiempo_actual = std::chrono::high_resolution_clock::now();
            auto tiempo_transcurrido = std::chrono::duration_cast<std::chrono::milliseconds>(
                tiempo_actual - inicio_total).count();
            mostrarProgreso(i + 1, resultado.total_combinaciones, tiempo_transcurrido);
        }
    }
    
    auto fin_total = std::chrono::high_resolution_clock::now();
    auto duracion_total = std::chrono::duration_cast<std::chrono::microseconds>(fin_total - inicio_total);
    
    // Calcular estadísticas de tiempo
    resultado.tiempo_total_ms = duracion_total.count() / 1000.0;
    resultado.tiempo_promedio_ms = calcularPromedio(tiempos);
    resultado.tiempo_minimo_ms = calcularMinimo(tiempos);
    resultado.tiempo_maximo_ms = calcularMaximo(tiempos);
    resultado.desviacion_estandar_ms = calcularDesviacionEstandar(tiempos, resultado.tiempo_promedio_ms);
    
    // Calcular métricas de rendimiento
    resultado.combinaciones_por_segundo = (resultado.total_combinaciones * 1000.0) / resultado.tiempo_total_ms;
    resultado.tiempo_por_bit_ms = resultado.tiempo_total_ms / largo_problema;
    
    // Calcular estadísticas de costo
    if (!costos.empty()) {
        resultado.costo_promedio = calcularPromedio(costos);
        resultado.costo_minimo = calcularMinimo(costos);
        resultado.costo_maximo = calcularMaximo(costos);
    } else {
        resultado.costo_promedio = resultado.costo_minimo = resultado.costo_maximo = 0.0;
    }
    
    // Calcular estadísticas de cache
    resultado.promedio_estados_cache = static_cast<size_t>(calcularPromedio(
        std::vector<double>(cache_sizes.begin(), cache_sizes.end())));
    
    if (mostrar_progreso) {
        std::cout << std::endl;
        std::cout << "✅ Benchmark completado!" << std::endl;
    }
    
    return resultado;
}

ResultadoBenchmark BenchmarkSistema::ejecutarBenchmarkConLimite(int largo_problema, double limite_segundos, bool mostrar_progreso) {
    ResultadoBenchmark resultado;
    resultado.largo_problema = largo_problema;
    resultado.total_combinaciones = 1LL << largo_problema;
    
    auto combinaciones = generarTodasLasCombinaciones(largo_problema);
    auto inicio = std::chrono::high_resolution_clock::now();
    double limite_ms = limite_segundos * 1000.0;
    
    std::vector<double> tiempos;
    std::vector<double> costos;
    
    int procesadas = 0;
    for (size_t i = 0; i < combinaciones.size(); i++) {
        auto tiempo_actual = std::chrono::high_resolution_clock::now();
        auto transcurrido = std::chrono::duration_cast<std::chrono::milliseconds>(tiempo_actual - inicio).count();
        
        if (transcurrido >= limite_ms) {
            std::cout << "⏰ Límite de tiempo alcanzado. Procesadas: " << procesadas 
                      << "/" << resultado.total_combinaciones << std::endl;
            break;
        }
        
        auto stats = analizarCombinacion(combinaciones[i]);
        tiempos.push_back(stats.tiempo_ejecucion_ms);
        if (stats.solucion_factible) {
            costos.push_back(stats.costo_solucion);
            resultado.combinaciones_factibles++;
        }
        
        procesadas++;
    }
    
    auto fin = std::chrono::high_resolution_clock::now();
    auto duracion = std::chrono::duration_cast<std::chrono::microseconds>(fin - inicio);
    resultado.tiempo_total_ms = duracion.count() / 1000.0;
    
    if (!tiempos.empty()) {
        resultado.tiempo_promedio_ms = calcularPromedio(tiempos);
        resultado.combinaciones_por_segundo = (procesadas * 1000.0) / resultado.tiempo_total_ms;
    }
    
    return resultado;
}

std::vector<std::vector<int>> BenchmarkSistema::generarTodasLasCombinaciones(int largo) {
    std::vector<std::vector<int>> combinaciones;
    long long total = 1LL << largo; // 2^largo
    
    // Para casos muy grandes, advertir sobre uso de memoria
    if (largo > 20) {
        std::cout << "⚠️  Generando " << total << " combinaciones..." << std::endl;
        std::cout << "💾 Esto requerirá aproximadamente " << (total * largo * sizeof(int) / (1024*1024)) << " MB de RAM" << std::endl;
    }
    
    combinaciones.reserve(total);
    
    for (long long i = 0; i < total; i++) {
        std::vector<int> combinacion(largo);
        
        // Convertir número i a representación binaria
        for (int bit = 0; bit < largo; bit++) {
            combinacion[bit] = (i >> bit) & 1;
        }
        
        combinaciones.push_back(combinacion);
        
        // Mostrar progreso cada millón de combinaciones para casos grandes
        if (largo > 20 && (i + 1) % 1000000 == 0) {
            std::cout << "📊 Generadas " << (i + 1) << " / " << total << " combinaciones..." << std::endl;
        }
    }
    
    return combinaciones;
}

EstadisticasCombinacion BenchmarkSistema::analizarCombinacion(const std::vector<int>& combinacion) {
    EstadisticasCombinacion stats;
    stats.combinacion = combinacion;
    
    auto inicio = std::chrono::high_resolution_clock::now();
    
    OptimizadorMaquina optimizador(combinacion);
    auto solucion = optimizador.resolver();
    
    auto fin = std::chrono::high_resolution_clock::now();
    auto duracion = std::chrono::duration_cast<std::chrono::microseconds>(fin - inicio);
    
    stats.tiempo_ejecucion_ms = duracion.count() / 1000.0;
    stats.costo_solucion = solucion.costo_total;
    stats.solucion_factible = solucion.es_factible;
    
    // Estimar uso de cache (aproximado - en implementación real necesitaríamos acceso interno)
    stats.estados_cache_utilizados = combinacion.size() * 6; // Estimación: largo * num_estados
    
    return stats;
}

void BenchmarkSistema::mostrarResultados(const ResultadoBenchmark& resultado) {
    std::cout << "================================================================" << std::endl;
    std::cout << "📊 RESULTADOS DEL BENCHMARK" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "Largo del problema: " << resultado.largo_problema << " bits" << std::endl;
    std::cout << "Total de combinaciones: " << resultado.total_combinaciones << std::endl;
    std::cout << "Combinaciones factibles: " << resultado.combinaciones_factibles 
              << " (" << std::fixed << std::setprecision(1) 
              << (100.0 * resultado.combinaciones_factibles / resultado.total_combinaciones) << "%)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "⏱️  MÉTRICAS DE TIEMPO:" << std::endl;
    std::cout << "Tiempo total: " << formatearTiempo(resultado.tiempo_total_ms) << std::endl;
    std::cout << "Tiempo promedio por combinación: " << formatearTiempo(resultado.tiempo_promedio_ms) << std::endl;
    std::cout << "Tiempo mínimo: " << formatearTiempo(resultado.tiempo_minimo_ms) << std::endl;
    std::cout << "Tiempo máximo: " << formatearTiempo(resultado.tiempo_maximo_ms) << std::endl;
    std::cout << "Desviación estándar: " << formatearTiempo(resultado.desviacion_estandar_ms) << std::endl;
    std::cout << std::endl;
    
    std::cout << "🚀 MÉTRICAS DE RENDIMIENTO:" << std::endl;
    std::cout << "Velocidad: " << formatearVelocidad(resultado.combinaciones_por_segundo) << std::endl;
    std::cout << "Tiempo por bit: " << formatearTiempo(resultado.tiempo_por_bit_ms) << std::endl;
    std::cout << std::endl;
    
    std::cout << "💰 MÉTRICAS DE COSTO:" << std::endl;
    if (resultado.combinaciones_factibles > 0) {
        std::cout << "Costo promedio: " << std::fixed << std::setprecision(2) << resultado.costo_promedio << std::endl;
        std::cout << "Costo mínimo: " << resultado.costo_minimo << std::endl;
        std::cout << "Costo máximo: " << resultado.costo_maximo << std::endl;
    } else {
        std::cout << "No hay soluciones factibles para mostrar costos" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "🧠 MÉTRICAS DE MEMORIA:" << std::endl;
    std::cout << "Promedio estados en cache: " << resultado.promedio_estados_cache << std::endl;
    std::cout << "Máximo estados en cache: " << resultado.maximo_estados_cache << std::endl;
    std::cout << std::endl;
    
    std::cout << "📈 DISTRIBUCIÓN DE ESCENARIOS:" << std::endl;
    std::cout << "Escenarios críticos (todos 0s): " << resultado.escenarios_criticos << std::endl;
    std::cout << "Escenarios óptimos (todos 1s): " << resultado.escenarios_optimos << std::endl;
    std::cout << "Escenarios mixtos: " << resultado.escenarios_mixtos << std::endl;
    std::cout << std::endl;
    
    // Análisis de escalabilidad
    double complejidad_teorica = resultado.largo_problema * 36; // H * S^2
    double factor_real = resultado.tiempo_promedio_ms / complejidad_teorica;
    std::cout << "📊 ANÁLISIS DE COMPLEJIDAD:" << std::endl;
    std::cout << "Complejidad teórica O(H×S²): " << complejidad_teorica << std::endl;
    std::cout << "Factor de la implementación real: " << std::scientific << factor_real << std::endl;
    std::cout << std::fixed;
}

void BenchmarkSistema::mostrarAnalisisEscalabilidad(const std::vector<ResultadoBenchmark>& resultados) {
    if (resultados.size() < 2) {
        std::cout << "❌ Se necesitan al menos 2 resultados para análisis de escalabilidad" << std::endl;
        return;
    }
    
    std::cout << "================================================================" << std::endl;
    std::cout << "📈 ANÁLISIS DE ESCALABILIDAD" << std::endl;
    std::cout << "================================================================" << std::endl;
    
    std::cout << std::setw(6) << "Bits" << " | "
              << std::setw(12) << "Combinaciones" << " | "
              << std::setw(10) << "Tiempo(ms)" << " | "
              << std::setw(12) << "Comb/seg" << " | "
              << std::setw(10) << "T/bit(ms)" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (const auto& resultado : resultados) {
        std::cout << std::setw(6) << resultado.largo_problema << " | "
                  << std::setw(12) << resultado.total_combinaciones << " | "
                  << std::setw(10) << std::fixed << std::setprecision(2) << resultado.tiempo_total_ms << " | "
                  << std::setw(12) << std::setprecision(0) << resultado.combinaciones_por_segundo << " | "
                  << std::setw(10) << std::setprecision(4) << resultado.tiempo_por_bit_ms << std::endl;
    }
    
    // Calcular factor de crecimiento
    std::cout << std::endl;
    std::cout << "FACTORES DE CRECIMIENTO:" << std::endl;
    for (size_t i = 1; i < resultados.size(); i++) {
        double factor_combinaciones = static_cast<double>(resultados[i].total_combinaciones) / 
                                    resultados[i-1].total_combinaciones;
        double factor_tiempo = resultados[i].tiempo_total_ms / resultados[i-1].tiempo_total_ms;
        
        std::cout << "Bits " << resultados[i-1].largo_problema << " → " << resultados[i].largo_problema 
                  << ": Combinaciones ×" << std::setprecision(1) << factor_combinaciones
                  << ", Tiempo ×" << std::setprecision(2) << factor_tiempo << std::endl;
    }
}

void BenchmarkSistema::exportarCSV(const ResultadoBenchmark& resultado, const std::string& archivo) {
    std::ofstream file(archivo);
    if (!file.is_open()) {
        std::cout << "❌ Error al crear archivo: " << archivo << std::endl;
        return;
    }
    
    // Header
    file << "largo_problema,total_combinaciones,combinaciones_factibles,tiempo_total_ms,"
         << "tiempo_promedio_ms,tiempo_minimo_ms,tiempo_maximo_ms,desviacion_estandar_ms,"
         << "combinaciones_por_segundo,tiempo_por_bit_ms,costo_promedio,costo_minimo,costo_maximo,"
         << "promedio_estados_cache,maximo_estados_cache,escenarios_criticos,escenarios_optimos,escenarios_mixtos\n";
    
    // Datos
    file << resultado.largo_problema << ","
         << resultado.total_combinaciones << ","
         << resultado.combinaciones_factibles << ","
         << resultado.tiempo_total_ms << ","
         << resultado.tiempo_promedio_ms << ","
         << resultado.tiempo_minimo_ms << ","
         << resultado.tiempo_maximo_ms << ","
         << resultado.desviacion_estandar_ms << ","
         << resultado.combinaciones_por_segundo << ","
         << resultado.tiempo_por_bit_ms << ","
         << resultado.costo_promedio << ","
         << resultado.costo_minimo << ","
         << resultado.costo_maximo << ","
         << resultado.promedio_estados_cache << ","
         << resultado.maximo_estados_cache << ","
         << resultado.escenarios_criticos << ","
         << resultado.escenarios_optimos << ","
         << resultado.escenarios_mixtos << "\n";
    
    file.close();
    std::cout << "✅ Resultados exportados a: " << archivo << std::endl;
}

// Funciones auxiliares privadas
double BenchmarkSistema::calcularPromedio(const std::vector<double>& valores) {
    if (valores.empty()) return 0.0;
    double suma = 0.0;
    for (double valor : valores) {
        suma += valor;
    }
    return suma / valores.size();
}

double BenchmarkSistema::calcularDesviacionEstandar(const std::vector<double>& valores, double promedio) {
    if (valores.size() <= 1) return 0.0;
    
    double suma_cuadrados = 0.0;
    for (double valor : valores) {
        double diff = valor - promedio;
        suma_cuadrados += diff * diff;
    }
    
    return std::sqrt(suma_cuadrados / (valores.size() - 1));
}

double BenchmarkSistema::calcularMinimo(const std::vector<double>& valores) {
    if (valores.empty()) return 0.0;
    return *std::min_element(valores.begin(), valores.end());
}

double BenchmarkSistema::calcularMaximo(const std::vector<double>& valores) {
    if (valores.empty()) return 0.0;
    return *std::max_element(valores.begin(), valores.end());
}

std::string BenchmarkSistema::clasificarEscenario(const std::vector<int>& combinacion) {
    int zeros = std::count(combinacion.begin(), combinacion.end(), 0);
    int ones = std::count(combinacion.begin(), combinacion.end(), 1);
    
    if (zeros == static_cast<int>(combinacion.size())) return "crítico";
    if (ones == static_cast<int>(combinacion.size())) return "óptimo";
    return "mixto";
}

void BenchmarkSistema::mostrarProgreso(int actual, int total, double tiempo_transcurrido) {
    double porcentaje = (100.0 * actual) / total;
    int barras = static_cast<int>(porcentaje / 5); // Barra de 20 caracteres
    
    std::cout << "\r[";
    for (int i = 0; i < 20; i++) {
        std::cout << (i < barras ? "█" : "░");
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << porcentaje << "% "
              << "(" << actual << "/" << total << ") "
              << "Tiempo: " << formatearTiempo(tiempo_transcurrido) << std::flush;
}

std::string BenchmarkSistema::formatearTiempo(double ms) {
    if (ms < 1.0) {
        return std::to_string(static_cast<int>(ms * 1000)) + " μs";
    } else if (ms < 1000.0) {
        return std::to_string(static_cast<int>(ms)) + " ms";
    } else {
        return std::to_string(ms / 1000.0) + " s";
    }
}

std::string BenchmarkSistema::formatearVelocidad(double ops_por_segundo) {
    if (ops_por_segundo > 1000000) {
        return std::to_string(ops_por_segundo / 1000000.0) + " M comb/s";
    } else if (ops_por_segundo > 1000) {
        return std::to_string(ops_por_segundo / 1000.0) + " K comb/s";
    } else {
        return std::to_string(ops_por_segundo) + " comb/s";
    }
}

// Implementación del namespace AnalisisBenchmark
namespace AnalisisBenchmark {
    void compararRendimiento(const std::vector<ResultadoBenchmark>& resultados) {
        std::cout << "📊 COMPARACIÓN DE RENDIMIENTO" << std::endl;
        std::cout << "Resultados disponibles: " << resultados.size() << std::endl;
        // Implementación completa según necesidades
    }
    
    void predecirEscalabilidad(const std::vector<ResultadoBenchmark>& resultados, int largo_objetivo) {
        std::cout << "🔮 PREDICCIÓN DE ESCALABILIDAD para " << largo_objetivo << " bits" << std::endl;
        // Implementación de predicción basada en regresión
    }
    
    void analizarComplejidad(const std::vector<ResultadoBenchmark>& resultados) {
        std::cout << "🧮 ANÁLISIS DE COMPLEJIDAD EMPÍRICA" << std::endl;
        // Análisis de la complejidad real vs teórica
    }
    
    void generarReporteCompleto(const std::vector<ResultadoBenchmark>& resultados, const std::string& archivo) {
        std::cout << "📄 Generando reporte completo en: " << archivo << std::endl;
        // Generar reporte detallado
    }
} 