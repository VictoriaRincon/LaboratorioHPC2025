#include "escenario.hpp"
#include "calculador_costos.hpp"
#include <iostream>

int main() {
    std::cout << "=== SISTEMA DE OPTIMIZACIÓN DE MÁQUINA DE ESTADOS ===" << std::endl;
    std::cout << "Modelando calentador con estados térmicos" << std::endl;
    
    // Crear el escenario y cargar datos
    Escenario escenario;
    
    if (!escenario.cargarDatos("data/parametros.in")) {
        std::cerr << "Error al cargar los datos del escenario." << std::endl;
        return 1;
    }
    
    // Mostrar los datos cargados
    escenario.mostrarDatos();
    
    // Crear el calculador de costos
    CalculadorCostos calculador(escenario);
    
    // Configurar costos de mantenimiento (ON_FRIO < ON_TIBIO < ON_CALIENTE)
    calculador.configurarCostos(1.0, 2.5, 5.0);  // Frio=1, Tibio=2.5, Caliente=5
    
    // Mostrar análisis detallado del escenario
    calculador.mostrarAnalisisDetallado();
    
    std::cout << "\n=== CONFIGURACIÓN DE COSTOS ===" << std::endl;
    std::cout << "ON/FRIO: 1.0" << std::endl;
    std::cout << "ON/TIBIO: 2.5" << std::endl;
    std::cout << "ON/CALIENTE: 5.0" << std::endl;
    std::cout << "Estados OFF: 0.0" << std::endl;
    
    std::cout << "\n=== MÁQUINA DE ESTADOS ===" << std::endl;
    std::cout << "Transiciones válidas:" << std::endl;
    std::cout << "ON/CALIENTE -> OFF/CALIENTE | ON/CALIENTE" << std::endl;
    std::cout << "OFF/CALIENTE -> ON/TIBIO | OFF/TIBIO" << std::endl;
    std::cout << "ON/TIBIO -> ON/CALIENTE | OFF/CALIENTE" << std::endl;
    std::cout << "OFF/TIBIO -> ON/FRIO | OFF/FRIO" << std::endl;
    std::cout << "ON/FRIO -> ON/TIBIO | OFF/TIBIO" << std::endl;
    std::cout << "OFF/FRIO -> ON/FRIO | OFF/FRIO" << std::endl;
    std::cout << "\nSolo ON/CALIENTE genera energía" << std::endl;
    
    // Resolver el problema
    Solucion solucion = calculador.resolver();
    
    // Mostrar la solución
    calculador.mostrarSolucion(solucion);
    
    if (solucion.es_valida) {
        std::cout << "\n=== RESUMEN ===" << std::endl;
        std::cout << "✓ Solución óptima encontrada" << std::endl;
        std::cout << "✓ Costo total mínimo: " << solucion.costo_total << std::endl;
        std::cout << "✓ Todas las transiciones son válidas según la máquina de estados" << std::endl;
        std::cout << "✓ La demanda se satisface en todas las horas" << std::endl;
    } else {
        std::cout << "\n=== RESUMEN ===" << std::endl;
        std::cout << "✗ No se encontró una solución válida" << std::endl;
        std::cout << "✗ Verifique los datos de entrada o las restricciones" << std::endl;
    }
    
    return 0;
}
