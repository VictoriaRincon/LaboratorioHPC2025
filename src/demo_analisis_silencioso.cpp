#include "calculador_costos.hpp"
#include "escenario.hpp"
#include <iostream>
#include <fstream>
#include <bitset>
#include <chrono>
#include <iomanip>

int main() {
    // Configuración silenciosa para procesamiento masivo
    std::cout << "=== ANALIZADOR MASIVO MODO SILENCIOSO ===\n";
    
    // Demanda fija según especificación del usuario
    std::vector<double> demanda_fija = {
        300, 200, 100, 100, 100, 200, 300, 500, 800, 1000, 1000, 1000,
        1000, 900, 800, 800, 800, 1000, 1000, 1000, 600, 600, 400, 300
    };
    
    uint32_t num_combinaciones;
    std::cin >> num_combinaciones;
    
    if (num_combinaciones > 16777216) {
        num_combinaciones = 16777216;
    }
    
    // Abrir archivo de resultados sin headers verbosos
    std::ofstream archivo_resultados("resultados_demo.csv");
    archivo_resultados << "CombinacionID,PatronEolica,CostoTotal,SolucionValida,HorasCriticas\n";
    
    auto inicio = std::chrono::steady_clock::now();
    double mejor_costo = std::numeric_limits<double>::infinity();
    uint32_t combinacion_optima = 0;
    uint32_t soluciones_validas = 0;
    double suma_costos = 0.0;
    
    // Estadísticas de progreso
    const uint32_t INTERVALO_REPORTE = std::max(100U, num_combinaciones / 100); // Cada 1% del total
    
    std::cout << "Procesando " << num_combinaciones << " combinaciones...\n";
    std::cout << "Progreso: [----------] 0%\n";
    
    for (uint32_t combinacion = 0; combinacion < num_combinaciones; combinacion++) {
        // Convertir número a patrón binario
        std::bitset<24> patron_eolica(combinacion);
        
        // Configurar escenario (SIN salida de debug)
        Escenario escenario;
        for (int hora = 0; hora < 24; hora++) {
            escenario.setDemanda(hora, demanda_fija[hora]);
            double energia_eolica = patron_eolica[hora] ? 500.0 : 0.0;
            escenario.setEnergiaOtrasFuentes(hora, energia_eolica);
        }
        
        // Resolver (redirigir salida a /dev/null para silenciar)
        CalculadorCostos calculador(escenario);
        calculador.configurarCostos(1.0, 2.5, 5.0);
        
        // Capturar stdout temporalmente
        std::cout.setstate(std::ios_base::failbit);
        Solucion solucion = calculador.resolver();
        std::cout.clear();
        
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
            suma_costos += solucion.costo_total;
            if (solucion.costo_total < mejor_costo) {
                mejor_costo = solucion.costo_total;
                combinacion_optima = combinacion;
            }
        }
        
        // Mostrar progreso condensado
        if ((combinacion + 1) % INTERVALO_REPORTE == 0 || combinacion + 1 == num_combinaciones) {
            auto ahora = std::chrono::steady_clock::now();
            auto duracion = std::chrono::duration_cast<std::chrono::seconds>(ahora - inicio);
            
            double porcentaje = (double)(combinacion + 1) / num_combinaciones * 100.0;
            double tasa = (double)(combinacion + 1) / duracion.count();
            double eta_segundos = (num_combinaciones - combinacion - 1) / tasa;
            
            // Barra de progreso visual
            int barras_completas = (int)(porcentaje / 10);
            std::string barra = "[";
            for (int i = 0; i < 10; i++) {
                barra += (i < barras_completas) ? "=" : "-";
            }
            barra += "]";
            
            std::cout << "\rProgreso: " << barra << " " 
                      << std::fixed << std::setprecision(1) << porcentaje << "% | "
                      << "Casos: " << (combinacion + 1) << "/" << num_combinaciones << " | "
                      << "Válidos: " << soluciones_validas << " | "
                      << "Mejor: " << std::fixed << std::setprecision(1) << mejor_costo << " | "
                      << "Tasa: " << std::fixed << std::setprecision(0) << tasa << " c/s | "
                      << "ETA: " << (int)(eta_segundos/60) << "m" << (int)eta_segundos%60 << "s"
                      << std::flush;
        }
    }
    
    auto fin = std::chrono::steady_clock::now();
    auto tiempo_total = std::chrono::duration_cast<std::chrono::seconds>(fin - inicio);
    
    std::cout << "\n\n=== PROCESAMIENTO COMPLETADO ===\n";
    std::cout << "Combinaciones procesadas: " << num_combinaciones << "\n";
    std::cout << "Tiempo total: " << tiempo_total.count() << " segundos\n";
    std::cout << "Tasa promedio: " << std::fixed << std::setprecision(1) 
              << (double)num_combinaciones / tiempo_total.count() << " combinaciones/segundo\n";
    std::cout << "Soluciones válidas: " << soluciones_validas 
              << " (" << std::fixed << std::setprecision(1) 
              << (double)soluciones_validas / num_combinaciones * 100.0 << "%)\n";
    std::cout << "Mejor costo encontrado: " << mejor_costo 
              << " (combinación #" << combinacion_optima << ")\n";
    
    if (soluciones_validas > 0) {
        double costo_promedio = suma_costos / soluciones_validas;
        std::cout << "Costo promedio: " << std::fixed << std::setprecision(2) << costo_promedio << "\n";
    }
    
    archivo_resultados.close();
    std::cout << "Resultados guardados en: resultados_demo.csv\n";
    
    return 0;
}
