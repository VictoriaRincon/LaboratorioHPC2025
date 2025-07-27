#include "analizador_exhaustivo.hpp"
#include <iostream>
#include <vector>
#include <string>

void mostrarMenu() {
    std::cout << "\n=== ANALIZADOR EXHAUSTIVO DE MÁQUINA DE ESTADOS ===\n";
    std::cout << "1. Ejecutar análisis completo (16,777,216 combinaciones)\n";
    std::cout << "2. Ejecutar análisis parcial (especificar rango)\n";
    std::cout << "3. Ejecutar prueba pequeña (primeras 1000 combinaciones)\n";
    std::cout << "4. Ejecutar prueba mediana (primeras 100,000 combinaciones)\n";
    std::cout << "5. Configurar parámetros y ejecutar\n";
    std::cout << "0. Salir\n";
    std::cout << "Selecciona una opción: ";
}

int main() {
    std::cout << "=== SISTEMA DE ANÁLISIS EXHAUSTIVO ===\n";
    std::cout << "Este programa analizará todas las combinaciones posibles de energía eólica\n";
    std::cout << "para encontrar patrones óptimos de operación de la máquina de estados.\n\n";
    
    // Configurar demanda fija (como especificó el usuario)
    std::vector<double> demanda_fija = {
        300, 200, 100, 100, 100, 200, 300, 500, 800, 1000, 1000, 1000,
        1000, 900, 800, 800, 800, 1000, 1000, 1000, 600, 600, 400, 300
    };
    
    std::cout << "Demanda configurada por hora:\n";
    for (int h = 0; h < 24; h++) {
        std::cout << "H" << h << ":" << demanda_fija[h] << " ";
        if ((h + 1) % 6 == 0) std::cout << "\n";
    }
    std::cout << "\n";
    std::cout << "Energía eólica: 0 o 500 para cada hora (2^24 = 16,777,216 combinaciones)\n\n";
    
    AnalizadorExhaustivo analizador;
    analizador.configurarDemanda(demanda_fija);
    
    int opcion;
    do {
        mostrarMenu();
        std::cin >> opcion;
        
        switch (opcion) {
            case 1: {
                std::cout << "\n⚠️  ADVERTENCIA: El análisis completo puede tomar DÍAS o SEMANAS.\n";
                std::cout << "¿Estás seguro? (s/N): ";
                char confirmar;
                std::cin >> confirmar;
                
                if (confirmar == 's' || confirmar == 'S') {
                    analizador.configurarArchivos("resultados_completos.csv", "log_completo.txt");
                    analizador.configurarReporte(10000, false, 100.0); // Solo guardar costos <= 100
                    analizador.ejecutarAnalisisCompleto();
                }
                break;
            }
            
            case 2: {
                uint32_t desde, hasta;
                std::cout << "Ingresa el rango de combinaciones:\n";
                std::cout << "Desde (0 - 16777215): ";
                std::cin >> desde;
                std::cout << "Hasta (0 - 16777215): ";
                std::cin >> hasta;
                
                if (desde >= hasta || hasta > 16777216) {
                    std::cout << "Rango inválido.\n";
                    break;
                }
                
                std::string archivo_res = "resultados_parcial_" + std::to_string(desde) + "_" + std::to_string(hasta) + ".csv";
                std::string archivo_log = "log_parcial_" + std::to_string(desde) + "_" + std::to_string(hasta) + ".txt";
                
                analizador.configurarArchivos(archivo_res, archivo_log);
                analizador.configurarReporte(1000, true); // Guardar todos los resultados
                analizador.ejecutarAnalisisParcial(desde, hasta);
                break;
            }
            
            case 3: {
                std::cout << "\nEjecutando prueba pequeña (1000 combinaciones)...\n";
                analizador.configurarArchivos("resultados_prueba_1k.csv", "log_prueba_1k.txt");
                analizador.configurarReporte(100, true);
                analizador.ejecutarAnalisisParcial(0, 1000);
                break;
            }
            
            case 4: {
                std::cout << "\nEjecutando prueba mediana (100,000 combinaciones)...\n";
                std::cout << "Esto tomará aproximadamente 1-2 horas.\n";
                analizador.configurarArchivos("resultados_prueba_100k.csv", "log_prueba_100k.txt");
                analizador.configurarReporte(5000, false, 80.0); // Solo costos <= 80
                analizador.ejecutarAnalisisParcial(0, 100000);
                break;
            }
            
            case 5: {
                uint32_t desde, hasta, intervalo;
                double umbral_costo;
                bool guardar_todas;
                char respuesta;
                
                std::cout << "\n=== CONFIGURACIÓN PERSONALIZADA ===\n";
                std::cout << "Desde: ";
                std::cin >> desde;
                std::cout << "Hasta: ";
                std::cin >> hasta;
                std::cout << "Intervalo de reporte: ";
                std::cin >> intervalo;
                std::cout << "Umbral de costo (solo guardar <= X): ";
                std::cin >> umbral_costo;
                std::cout << "¿Guardar todas las soluciones? (s/N): ";
                std::cin >> respuesta;
                guardar_todas = (respuesta == 's' || respuesta == 'S');
                
                std::string archivo_res = "resultados_custom_" + std::to_string(desde) + "_" + std::to_string(hasta) + ".csv";
                std::string archivo_log = "log_custom_" + std::to_string(desde) + "_" + std::to_string(hasta) + ".txt";
                
                analizador.configurarArchivos(archivo_res, archivo_log);
                analizador.configurarReporte(intervalo, guardar_todas, umbral_costo);
                analizador.ejecutarAnalisisParcial(desde, hasta);
                break;
            }
            
            case 0:
                std::cout << "¡Análisis terminado!\n";
                break;
                
            default:
                std::cout << "Opción inválida.\n";
                break;
        }
        
    } while (opcion != 0);
    
    std::cout << "\n=== INFORMACIÓN DE ARCHIVOS GENERADOS ===\n";
    std::cout << "Los resultados se guardan en formato CSV con las columnas:\n";
    std::cout << "- CombinacionID: Identificador único (0 a 16777215)\n";
    std::cout << "- PatronEolica: Patrón binario de 24 bits (0=no eólica, 1=eólica)\n";
    std::cout << "- CostoTotal: Costo óptimo encontrado\n";
    std::cout << "- SolucionValida: SI/NO\n";
    std::cout << "- HorasCriticas: Número de horas que requieren generación\n";
    std::cout << "- SecuenciaEstados: Secuencia óptima de estados por hora\n\n";
    
    std::cout << "Los logs contienen información de progreso y estadísticas detalladas.\n\n";
    std::cout << "¡Gracias por usar el Analizador Exhaustivo!\n";
    
    return 0;
}
