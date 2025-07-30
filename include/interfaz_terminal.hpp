#ifndef INTERFAZ_TERMINAL_HPP
#define INTERFAZ_TERMINAL_HPP

#include "optimizador.hpp"
#include "benchmark_sistema.hpp"
#include <string>

class InterfazTerminal {
public:
    // Ejecutar la interfaz principal
    static void ejecutar();
    
    // Mostrar menú principal
    static void mostrarMenu();
    
    // Procesar argumentos de línea de comandos
    static void procesarArgumentos(int argc, char* argv[]);

private:
    // Opciones del menú
    static void cargarArchivoParametros();
    static void ingresarParametrosManual();
    static void ejecutarPruebaRapida();
    static void ejecutarPruebasValidacion();
    static void ejecutarBenchmarkRendimiento();
    static void ejecutarBenchmarkEscalabilidad();
    static void mostrarAyuda();
    
    // Funciones auxiliares
    static std::vector<int> leerArchivoParametros(const std::string& archivo);
    static std::vector<int> parsearParametrosString(const std::string& entrada);
    static void mostrarSolucion(const Solucion& solucion, const std::vector<int>& energia_eolica);
    static void mostrarEstadisticasDetalladas(const std::vector<int>& energia_eolica);
    static void validarSolucion(const Solucion& solucion, const std::vector<int>& energia_eolica);
    
    // Pruebas predefinidas
    static void ejecutarPruebaCritica();
    static void ejecutarPruebaOptima();
    static void ejecutarPruebaMixta();
    static void ejecutarPruebaCompleja();
    
    // Utilidades de entrada/salida
    static std::string leerEntrada(const std::string& prompt);
    static int leerEntero(const std::string& prompt, int min_val = 0, int max_val = 1000);
    static bool confirmar(const std::string& mensaje);
    
    // Formateo y visualización
    static void imprimirSeparador(char caracter = '=', int longitud = 60);
    static void imprimirTitulo(const std::string& titulo);
    static std::string formatearTiempo(double segundos);
};

#endif // INTERFAZ_TERMINAL_HPP 