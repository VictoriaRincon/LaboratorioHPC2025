#include "../include/calculador_costos.hpp"
#include <iostream>

int main()
{
  auto inicio = std::chrono::high_resolution_clock::now();

  std::cout << "Ejecutando el Calculador de Costos..." << std::endl;
  for (double eo = 0; eo < 1464; eo++) //1464
  {
    std::cout << "Energia eolica: " << eo << std::endl;
    calcular_costo(eo);
  }
  auto fin = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duracion = fin - inicio;
  std::cout << "Tiempo transcurrido: " << duracion.count() << " segundos" << std::endl;
  return 0;
}

// int main() {
//     int horas = 24;
//     std::cout << "Valor de horas: " << horas << std::endl;

// std::vector<bool>* encender = new std::vector<bool>(horas, false);
//     std::cout << "Vector creado OK\n";

//     return 0;
// }