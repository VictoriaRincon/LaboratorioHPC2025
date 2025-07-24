#include "../include/calculador_costos.hpp"
#include <iostream>

int main() {
  std::cout << "Ejecutando el Calculador de Costos..." << std::endl;
  struct Escenario escenario = {
    00, // hora
    0.0 // energia_eolica
  };
  calcular_costo(escenario);
  return 0;
}

// int main() {
//     int horas = 24;
//     std::cout << "Valor de horas: " << horas << std::endl;

// std::vector<bool>* encender = new std::vector<bool>(horas, false);
//     std::cout << "Vector creado OK\n";

//     return 0;
// }