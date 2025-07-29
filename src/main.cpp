#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include "../include/escenario.hpp"
#include "../include/calculador_costos.hpp"

// Declaraciones de funciones
void mostrarMenu();
void mostrarResultado(const ResultadoOptimizacion& resultado, const Escenario& escenario);
std::vector<int> leerArrayManual();
void ejecutarTestsPredefinidos();
void configurarOpciones(CalculadorCostos& calculador);
void validarPropiedadSubsecuencias();
void compararVersiones();

void mostrarMenu() {
    std::cout << "\n=== SISTEMA DE OPTIMIZACIÓN DE MÁQUINA DE ESTADOS ===" << std::endl;
    std::cout << "1. Cargar escenario desde archivo" << std::endl;
    std::cout << "2. Introducir escenario manual" << std::endl;
    std::cout << "3. Usar escenario de ejemplo" << std::endl;
    std::cout << "4. Ejecutar tests predefinidos" << std::endl;
    std::cout << "5. Configurar opciones" << std::endl;
    std::cout << "6. Validar propiedad de subsecuencias" << std::endl;
    std::cout << "7. Comparar versión original vs optimizada" << std::endl;
    std::cout << "0. Salir" << std::endl;
    std::cout << "Seleccione una opción: ";
}

void mostrarResultado(const ResultadoOptimizacion& resultado, const Escenario& escenario) {
    std::cout << "\n=== SOLUCIÓN ENCONTRADA ===" << std::endl;
    
    if (!resultado.solucionValida) {
        std::cout << "❌ No se encontró una solución válida" << std::endl;
        return;
    }
    
    std::cout << "Costo total: " << std::fixed << std::setprecision(1) << resultado.costoTotal << std::endl;
    std::cout << "Horizonte temporal: " << resultado.horasOptimizadas << " horas (0-" << (resultado.horasOptimizadas - 1) << ")" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Estados por hora:" << std::endl;
    std::cout << std::left << std::setw(6) << "Hora" 
              << std::setw(16) << "Estado" 
              << std::setw(8) << "Costo" 
              << std::setw(8) << "Eólica" 
              << std::setw(13) << "Requiere_Gen" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    for (size_t i = 0; i < resultado.estados.size(); ++i) {
        Estado estado = resultado.estados[i];
        double costo = CalculadorCostos::getCostoEstado(estado);
        bool eolicaSuficiente = escenario.esEolicaSuficiente(static_cast<int>(i));
        bool requiereGen = !eolicaSuficiente;
        
        std::cout << std::left << std::setw(6) << i
                  << std::setw(16) << CalculadorCostos::estadoAString(estado)
                  << std::setw(8) << std::fixed << std::setprecision(1) << costo
                  << std::setw(8) << (eolicaSuficiente ? "1" : "0")
                  << std::setw(13) << (requiereGen ? "Sí" : "No") << std::endl;
    }
    std::cout << std::endl;
}

std::vector<int> leerArrayManual() {
    std::cout << "Ingrese los valores binarios separados por espacios (0 o 1): ";
    std::string linea;
    std::getline(std::cin, linea);
    
    std::vector<int> valores;
    std::istringstream iss(linea);
    int valor;
    
    while (iss >> valor) {
        if (valor == 0 || valor == 1) {
            valores.push_back(valor);
        } else {
            std::cout << "Advertencia: Valor " << valor << " no es válido (debe ser 0 o 1), ignorado." << std::endl;
        }
    }
    
    return valores;
}

void ejecutarTestsPredefinidos() {
    CalculadorCostos calculador;
    
    std::cout << "\n=== EJECUTANDO TESTS PREDEFINIDOS ===" << std::endl;
    
    // Test 1: Escenario crítico
    std::cout << "\nTest 1: Escenario Crítico (todos 0s)" << std::endl;
    Escenario test1;
    test1.cargarDesdeArray({0, 0, 0, 0, 0});
    test1.mostrarEstadisticas();
    auto resultado1 = calculador.resolver(test1);
    mostrarResultado(resultado1, test1);
    
    // Test 2: Escenario óptimo
    std::cout << "\nTest 2: Escenario Óptimo (todos 1s)" << std::endl;
    Escenario test2;
    test2.cargarDesdeArray({1, 1, 1, 1, 1});
    test2.mostrarEstadisticas();
    auto resultado2 = calculador.resolver(test2);
    mostrarResultado(resultado2, test2);
    
    // Test 3: Escenario mixto
    std::cout << "\nTest 3: Escenario Mixto" << std::endl;
    Escenario test3;
    test3.cargarDesdeArray({1, 1, 0, 0, 1, 0, 1});
    test3.mostrarEstadisticas();
    auto resultado3 = calculador.resolver(test3);
    mostrarResultado(resultado3, test3);
    
    // Test 4: Caso más largo
    std::cout << "\nTest 4: Escenario largo (12 horas)" << std::endl;
    Escenario test4;
    test4.cargarDesdeArray({1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1});
    test4.mostrarEstadisticas();
    auto resultado4 = calculador.resolver(test4);
    mostrarResultado(resultado4, test4);
}

void configurarOpciones(CalculadorCostos& calculador) {
    std::cout << "\n=== CONFIGURACIÓN ===" << std::endl;
    std::cout << "1. Activar/desactivar detalles de debugging" << std::endl;
    std::cout << "Seleccione opción: ";
    
    int opcion;
    std::cin >> opcion;
    
    if (opcion == 1) {
        std::cout << "¿Mostrar detalles? (1=Sí, 0=No): ";
        int mostrar;
        std::cin >> mostrar;
        calculador.setMostrarDetalles(mostrar == 1);
        std::cout << "Configuración actualizada." << std::endl;
    }
}

void validarPropiedadSubsecuencias() {
    std::cout << "\n=== VALIDACIÓN DE PROPIEDAD DE SUBSECUENCIAS ===" << std::endl;
    std::cout << "Propiedad: Cualquier tira entre dos 0's tiene la misma solución óptima" << std::endl;
    
    CalculadorCostos calculador;
    
    // Test 1: Secuencia original
    std::cout << "\nTest 1: Secuencia original {0,1,1,1,0}" << std::endl;
    Escenario test1;
    test1.cargarDesdeArray({0, 1, 1, 1, 0});
    auto resultado1 = calculador.resolver(test1);
    
    if (resultado1.solucionValida) {
        std::cout << "Estados: ";
        for (size_t i = 0; i < resultado1.estados.size(); ++i) {
            std::cout << CalculadorCostos::estadoAString(resultado1.estados[i]);
            if (i < resultado1.estados.size() - 1) std::cout << " -> ";
        }
        std::cout << std::endl;
        std::cout << "Costo: " << resultado1.costoTotal << std::endl;
    }
    
    // Test 2: Secuencia con contexto adicional
    std::cout << "\nTest 2: Secuencia con contexto {1,0,1,1,1,0}" << std::endl;
    Escenario test2;
    test2.cargarDesdeArray({1, 0, 1, 1, 1, 0});
    auto resultado2 = calculador.resolver(test2);
    
    if (resultado2.solucionValida) {
        std::cout << "Estados completos: ";
        for (size_t i = 0; i < resultado2.estados.size(); ++i) {
            std::cout << CalculadorCostos::estadoAString(resultado2.estados[i]);
            if (i < resultado2.estados.size() - 1) std::cout << " -> ";
        }
        std::cout << std::endl;
        std::cout << "Costo: " << resultado2.costoTotal << std::endl;
        
        // Extraer subsecuencia entre 0's (posiciones 1-5: {0,1,1,1,0})
        std::cout << "Subsecuencia extraída (pos 1-5): ";
        for (size_t i = 1; i < 6; ++i) {
            std::cout << CalculadorCostos::estadoAString(resultado2.estados[i]);
            if (i < 5) std::cout << " -> ";
        }
        std::cout << std::endl;
    }
    
    // Test 3: Resolver solo la subsecuencia {0,1,1,1,0} de forma aislada otra vez
    std::cout << "\nTest 3: Resolver subsecuencia aislada nuevamente {0,1,1,1,0}" << std::endl;
    Escenario test3;
    test3.cargarDesdeArray({0, 1, 1, 1, 0});
    auto resultado3 = calculador.resolver(test3);
    
    if (resultado3.solucionValida) {
        std::cout << "Estados: ";
        for (size_t i = 0; i < resultado3.estados.size(); ++i) {
            std::cout << CalculadorCostos::estadoAString(resultado3.estados[i]);
            if (i < resultado3.estados.size() - 1) std::cout << " -> ";
        }
        std::cout << std::endl;
        std::cout << "Costo: " << resultado3.costoTotal << std::endl;
    }
    
    // Análisis de la propiedad
    std::cout << "\n=== ANÁLISIS DE LA PROPIEDAD ===" << std::endl;
    
    if (resultado1.solucionValida && resultado2.solucionValida && resultado3.solucionValida) {
        // Comparar subsecuencias
        bool subsecuencia2_igual = true;
        bool test1_igual_test3 = true;
        
        // Comparar test1 vs subsecuencia en test2 (posiciones 1-5)
        for (int i = 0; i < 5; ++i) {
            if (resultado1.estados[i] != resultado2.estados[i + 1]) {
                subsecuencia2_igual = false;
            }
        }
        
        // Comparar test1 vs test3 (ambos son {0,1,1,1,0})
        for (int i = 0; i < 5; ++i) {
            if (resultado1.estados[i] != resultado3.estados[i]) {
                test1_igual_test3 = false;
            }
        }
        
        std::cout << "¿Subsecuencia {0,1,1,1,0} en test2 igual a test1? " 
                  << (subsecuencia2_igual ? "✅ SÍ" : "❌ NO") << std::endl;
        std::cout << "¿Test3 aislado igual a test1? " 
                  << (test1_igual_test3 ? "✅ SÍ" : "❌ NO") << std::endl;
        
        if (subsecuencia2_igual && test1_igual_test3) {
            std::cout << "\n🎉 PROPIEDAD VALIDADA: Las subsecuencias {0,1,1,1,0} son idénticas" << std::endl;
            std::cout << "💡 Esto permite optimización por reutilización de soluciones!" << std::endl;
        } else {
            std::cout << "\n⚠️  PROPIEDAD NO VALIDADA: Las subsecuencias difieren" << std::endl;
        }
        
        // Mostrar diferencias si las hay
        if (!subsecuencia2_igual || !test1_igual_test3) {
            std::cout << "\nDETALLE DE DIFERENCIAS:" << std::endl;
            for (int i = 0; i < 5; ++i) {
                std::cout << "Pos " << i << ": Test1=" << CalculadorCostos::estadoAString(resultado1.estados[i]);
                if (!subsecuencia2_igual) {
                    std::cout << " | Test2[" << (i+1) << "]=" << CalculadorCostos::estadoAString(resultado2.estados[i + 1]);
                }
                if (!test1_igual_test3) {
                    std::cout << " | Test3=" << CalculadorCostos::estadoAString(resultado3.estados[i]);
                }
                std::cout << std::endl;
            }
        }
    }
    
    // Test adicional: Múltiples subsecuencias
    std::cout << "\n=== TEST ADICIONAL: MÚLTIPLES SUBSECUENCIAS ===" << std::endl;
    std::cout << "Test 4: Secuencia con múltiples subsecuencias {0,1,0,1,1,0}" << std::endl;
    Escenario test4;
    test4.cargarDesdeArray({0, 1, 0, 1, 1, 0});
    auto resultado4 = calculador.resolver(test4);
    
    if (resultado4.solucionValida) {
        mostrarResultado(resultado4, test4);
        
        // Identificar subsecuencias
        std::cout << "Subsecuencias identificadas:" << std::endl;
        std::cout << "1. Pos 0-2: {0,1,0}" << std::endl;
        std::cout << "2. Pos 2-5: {0,1,1,0}" << std::endl;
    }
}

void compararVersiones() {
    std::cout << "\n=== COMPARACIÓN: ORIGINAL VS OPTIMIZADA ===" << std::endl;
    
    CalculadorCostos calculador;
    calculador.setMostrarDetalles(true);
    
    // Test con múltiples subsecuencias que se repiten
    std::vector<int> testComplejo = {
        1, 0, 1, 1, 0,     // Primera subsecuencia {0,1,1,0}
        1, 1, 0, 1, 1, 0,  // Segunda subsecuencia {0,1,1,0} (repetida)
        0, 1, 0,           // Tercera subsecuencia {0,1,0}
        1, 0, 1, 0,        // Cuarta subsecuencia {0,1,0} (repetida)
        1, 1
    };
    
    Escenario escenario;
    escenario.cargarDesdeArray(testComplejo);
    
    std::cout << "Patrón de prueba: ";
    for (int val : testComplejo) std::cout << val << " ";
    std::cout << "\n" << std::endl;
    
    escenario.mostrarEstadisticas();
    
    // Versión original
    std::cout << "\n=== VERSIÓN ORIGINAL ===" << std::endl;
    auto inicio = std::chrono::high_resolution_clock::now();
    auto resultadoOriginal = calculador.resolver(escenario);
    auto fin = std::chrono::high_resolution_clock::now();
    auto tiempoOriginal = std::chrono::duration_cast<std::chrono::microseconds>(fin - inicio);
    
    std::cout << "Tiempo: " << tiempoOriginal.count() << " microsegundos" << std::endl;
    if (resultadoOriginal.solucionValida) {
        std::cout << "Costo: " << resultadoOriginal.costoTotal << std::endl;
    }
    
    // Versión optimizada
    std::cout << "\n=== VERSIÓN OPTIMIZADA CON CACHÉ ===" << std::endl;
    calculador.limpiarCachePatrones();
    inicio = std::chrono::high_resolution_clock::now();
    auto resultadoOptimizado = calculador.resolverConPatrones(escenario);
    fin = std::chrono::high_resolution_clock::now();
    auto tiempoOptimizado = std::chrono::duration_cast<std::chrono::microseconds>(fin - inicio);
    
    std::cout << "Tiempo: " << tiempoOptimizado.count() << " microsegundos" << std::endl;
    if (resultadoOptimizado.solucionValida) {
        std::cout << "Costo: " << resultadoOptimizado.costoTotal << std::endl;
    }
    
    // Mostrar estadísticas del caché
    calculador.mostrarEstadisticasCache();
    
    // Comparación de resultados
    std::cout << "\n=== COMPARACIÓN DE RESULTADOS ===" << std::endl;
    
    bool solucionesIguales = true;
    if (resultadoOriginal.solucionValida && resultadoOptimizado.solucionValida) {
        if (resultadoOriginal.estados.size() == resultadoOptimizado.estados.size()) {
            for (size_t i = 0; i < resultadoOriginal.estados.size(); ++i) {
                if (resultadoOriginal.estados[i] != resultadoOptimizado.estados[i]) {
                    solucionesIguales = false;
                    break;
                }
            }
        } else {
            solucionesIguales = false;
        }
        
        std::cout << "¿Soluciones idénticas? " << (solucionesIguales ? "✅ SÍ" : "❌ NO") << std::endl;
        std::cout << "Costo original: " << resultadoOriginal.costoTotal << std::endl;
        std::cout << "Costo optimizado: " << resultadoOptimizado.costoTotal << std::endl;
        
        if (tiempoOriginal.count() > 0) {
            double speedup = static_cast<double>(tiempoOriginal.count()) / tiempoOptimizado.count();
            std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
        }
    }
    
    // Test de reutilización con segundo problema
    std::cout << "\n=== TEST DE REUTILIZACIÓN ===" << std::endl;
    std::vector<int> testReutilizacion = {
        0, 1, 1, 0,        // Patrón ya en caché
        1, 1, 1,           // Solo 1s
        0, 1, 0,           // Patrón ya en caché
        1, 0, 1, 1, 0      // Patrón ya en caché
    };
    
    Escenario escenario2;
    escenario2.cargarDesdeArray(testReutilizacion);
    
    std::cout << "Segundo patrón: ";
    for (int val : testReutilizacion) std::cout << val << " ";
    std::cout << std::endl;
    
    calculador.setMostrarDetalles(false);
    auto resultado2 = calculador.resolverConPatrones(escenario2);
    calculador.mostrarEstadisticasCache();
    
    if (resultado2.solucionValida) {
        std::cout << "Costo segundo problema: " << resultado2.costoTotal << std::endl;
    }
}

int main() {
    CalculadorCostos calculador;
    Escenario escenario;
    bool escenarioCargado = false;
    
    int opcion;
    
    do {
        mostrarMenu();
        std::cin >> opcion;
        std::cin.ignore(); // Limpiar buffer
        
        switch (opcion) {
            case 1: {
                std::cout << "Ingrese el nombre del archivo (ej: data/parametros.in): ";
                std::string archivo;
                std::getline(std::cin, archivo);
                
                if (escenario.cargarDesdeArchivo(archivo)) {
                    std::cout << "✅ Escenario cargado exitosamente." << std::endl;
                    escenario.mostrarEstadisticas();
                    escenarioCargado = true;
                } else {
                    std::cout << "❌ Error al cargar el archivo." << std::endl;
                }
                break;
            }
            
            case 2: {
                std::cout << "Introducir escenario manual:" << std::endl;
                std::cout << "Formato: valores binarios (0=necesita generación, 1=eólica suficiente)" << std::endl;
                auto valores = leerArrayManual();
                
                if (!valores.empty()) {
                    escenario.cargarDesdeArray(valores);
                    std::cout << "✅ Escenario cargado exitosamente." << std::endl;
                    escenario.mostrarEstadisticas();
                    escenarioCargado = true;
                } else {
                    std::cout << "❌ No se ingresaron valores válidos." << std::endl;
                }
                break;
            }
            
            case 3: {
                // Ejemplo del README
                escenario.cargarDesdeArray({1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1});
                std::cout << "✅ Escenario de ejemplo cargado (24 horas)." << std::endl;
                escenario.mostrarEstadisticas();
                escenarioCargado = true;
                break;
            }
            
            case 4: {
                ejecutarTestsPredefinidos();
                break;
            }
            
            case 5: {
                configurarOpciones(calculador);
                break;
            }
            
            case 6: {
                validarPropiedadSubsecuencias();
                break;
            }
            
            case 7: {
                compararVersiones();
                break;
            }
            
            case 0:
                std::cout << "¡Hasta luego!" << std::endl;
                break;
                
            default:
                std::cout << "Opción no válida." << std::endl;
                continue;
        }
        
        // Si hay un escenario cargado, preguntar qué versión usar
        if (escenarioCargado && opcion >= 1 && opcion <= 3) {
            std::cout << "\n¿Qué versión desea usar?" << std::endl;
            std::cout << "1. Versión original" << std::endl;
            std::cout << "2. Versión optimizada con caché" << std::endl;
            std::cout << "3. No resolver" << std::endl;
            std::cout << "Seleccione: ";
            
            int versionOpcion;
            std::cin >> versionOpcion;
            
            if (versionOpcion == 1) {
                std::cout << "Resolviendo con versión original..." << std::endl;
                auto resultado = calculador.resolver(escenario);
                mostrarResultado(resultado, escenario);
            } else if (versionOpcion == 2) {
                std::cout << "Resolviendo con versión optimizada..." << std::endl;
                auto resultado = calculador.resolverConPatrones(escenario);
                mostrarResultado(resultado, escenario);
                calculador.mostrarEstadisticasCache();
            }
        }
        
    } while (opcion != 0);
    
    return 0;
} 