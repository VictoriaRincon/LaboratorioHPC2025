#include "calculador_costos.hpp"
#include "escenario.hpp"
#include <iostream>
#include <fstream>
#include <bitset>
#include <chrono>
#include <iomanip>

int main() {
    std::cout << "=== DEMO DE ANÁLISIS EXHAUSTIVO ===\n";
    std::cout << "Analizando combinaciones de energía eólica para optimización\n\n";
    
    // Demanda fija según especificación del usuario
    std::vector<double> demanda_fija = {
        300, 200, 100, 100, 100, 200, 300, 500, 800, 1000, 1000, 1000,
        1000, 900, 800, 800, 800, 1000, 1000, 1000, 600, 600, 400, 300
    };
    
    std::cout << "Demanda configurada por hora:\n";
    for (int h = 0; h < 24; h++) {
        std::cout << "H" << h << ":" << demanda_fija[h] << " ";
        if ((h + 1) % 6 == 0) std::cout << "\n";
    }
    
    std::cout << "\n¿Cuántas combinaciones quieres analizar? (ej: 1000): ";
    uint32_t num_combinaciones;
    std::cin >> num_combinaciones;
    
    if (num_combinaciones > 16777216) {
        num_combinaciones = 16777216;
        std::cout << "Limitado a 16,777,216 combinaciones máximo.\n";
    }
    
    // Abrir archivo de resultados
    std::ofstream archivo_resultados("resultados_demo.csv");
    archivo_resultados << "CombinacionID,PatronEolica,CostoTotal,SolucionValida,HorasCriticas\n";
    
    auto inicio = std::chrono::steady_clock::now();
    double mejor_costo = std::numeric_limits<double>::infinity();
    uint32_t combinacion_optima = 0;
    uint32_t soluciones_validas = 0;
    
    std::cout << "\n=== INICIANDO ANÁLISIS ===\n";
    std::cout << "Procesando " << num_combinaciones << " combinaciones...\n\n";
    
    for (uint32_t combinacion = 0; combinacion < num_combinaciones; combinacion++) {
        // Convertir número a patrón binario
        std::bitset<24> patron_eolica(combinacion);
        
        // Configurar escenario
        Escenario escenario;
        for (int hora = 0; hora < 24; hora++) {
            escenario.setDemanda(hora, demanda_fija[hora]);
            double energia_eolica = patron_eolica[hora] ? 500.0 : 0.0;
            escenario.setEnergiaOtrasFuentes(hora, energia_eolica);
        }
        
        // Resolver
        CalculadorCostos calculador(escenario);
        calculador.configurarCostos(1.0, 2.5, 5.0);
        Solucion solucion = calculador.resolver();
        
        // Contar horas críticas
        int horas_criticas = 0;
        for (int hora = 0; hora < 24; hora++) {
            if (!escenario.demandaCubiertaConEO(hora)) {
                horas_criticas++;
            }
        }
        
        // Guardar resultado
        archivo_resultados << combinacion << "," 
                          << patron_eolica << ","
                          << std::fixed << std::setprecision(2) << solucion.costo_total << ","
                          << (solucion.es_valida ? "SI" : "NO") << ","
                          << horas_criticas << "\n";
        
        if (solucion.es_valida) {
            soluciones_validas++;
            if (solucion.costo_total < mejor_costo) {
                mejor_costo = solucion.costo_total;
                combinacion_optima = combinacion;
            }
        }
        
        // Mostrar progreso cada 100 combinaciones
        if ((combinacion + 1) % 100 == 0) {
            double porcentaje = (double)(combinacion + 1) / num_combinaciones * 100.0;
            auto ahora = std::chrono::steady_clock::now();
            auto duracion = std::chrono::duration_cast<std::chrono::seconds>(ahora - inicio);
            
            std::cout << "\r[" << std::fixed << std::setprecision(1) << porcentaje << "%] "
                      << (combinacion + 1) << "/" << num_combinaciones
                      << " | Válidas: " << soluciones_validas
                      << " | Mejor costo: " << mejor_costo
                      << " | Tiempo: " << duracion.count() << "s" << std::flush;
        }
    }
    
    auto fin = std::chrono::steady_clock::now();
    auto tiempo_total = std::chrono::duration_cast<std::chrono::seconds>(fin - inicio);
    
    std::cout << "\n\n=== ANÁLISIS COMPLETADO ===\n";
    std::cout << "Combinaciones analizadas: " << num_combinaciones << "\n";
    std::cout << "Soluciones válidas: " << soluciones_validas 
              << " (" << std::fixed << std::setprecision(2) 
              << (double)soluciones_validas / num_combinaciones * 100.0 << "%)\n";
    std::cout << "Mejor costo encontrado: " << mejor_costo << "\n";
    std::cout << "Combinación óptima: " << combinacion_optima 
              << " (binario: " << std::bitset<24>(combinacion_optima) << ")\n";
    std::cout << "Tiempo total: " << tiempo_total.count() << " segundos\n";
    std::cout << "Tasa de procesamiento: " << std::fixed << std::setprecision(1) 
              << (double)num_combinaciones / tiempo_total.count() << " combinaciones/segundo\n";
    
    archivo_resultados.close();
    std::cout << "\nResultados guardados en: resultados_demo.csv\n";
    
    return 0;
}
