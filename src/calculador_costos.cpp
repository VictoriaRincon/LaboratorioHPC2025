#include "../include/escenario.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

enum EstadoMaquina { APAGADA, PRENDIDA };

const double BASE_COSTO_ARRANQUE = 4208.0;
const double MULT_COSTO_GN = 511.0;
const double DENSIDAD_GN = 0.717;    // kg/m3 (valor aproximado)
const double CONSUMO_GN_GPKWH = 167; // gramos de GN por kWh
const double CONSUMO_GN_KGPKWH = CONSUMO_GN_GPKWH / 1000.0; // g/kWh a kg/kWh
const double CONSUMO_GN_M3PKWH = CONSUMO_GN_KGPKWH / DENSIDAD_GN;
const double COSTO_GN_USD_M3 = 618;
const double COSTO_ARRANQUE =
    BASE_COSTO_ARRANQUE + MULT_COSTO_GN * COSTO_GN_USD_M3;

struct Demanda {
  double hora;
  double valor;
};

// Función que determina el tipo de arranque
int obtener_minutos_apagada(const vector<bool> &encendida, int hora_actual) {
  int minutos = 0;
  for (int i = hora_actual + 1; i < encendida.size(); ++i) {
    if (encendida[i])
      break;
    minutos += 60;
  }
  return minutos;
}

// double calcular_costo_arranque() {
//   return BASE_COSTO_ARRANQUE + MULT_COSTO_GN * COSTO_GN_USD_M3;
// }

vector<Demanda> obtener_demanda() {
  std::string ruta = "data/parametros.in";
  std::ifstream archivo(ruta);
  std::vector<Demanda> datos;
  std::string linea;
  std::getline(archivo, linea);

  while (std::getline(archivo, linea)) {

    std::stringstream ss(linea);
    std::string hora, valor_str;
    std::getline(ss, hora, ',');
    std::getline(ss, valor_str, ',');

    Demanda demanda;
    demanda.hora = std::stod(hora);
    demanda.valor = std::stod(valor_str);
    datos.push_back(demanda);
  }

  return datos;
}

vector<bool> *calcular_costo(const Escenario &esc) {

  vector<Demanda> demanda =
      obtener_demanda(); // Puedo leer un archivo o que sea un arreglo de 24

  // bool necesidadEncender = (demanda.end()->valor - esc.energia_eolica) > 0.0;

  int horas = demanda.size();
  std::vector<bool> *encender = new std::vector<bool>(horas, false);
  std::vector<bool> *estado_maquina = new std::vector<bool>(horas, false);

  // Simulación hacia atrás
  for (int h = horas - 1; h >= 0; --h) {
    std::cout << "Hora:" + std::to_string(h) << std::endl;
    std::cout << "Demanda:" + std::to_string(demanda[h].valor) << std::endl;

    if (demanda[h].valor <= 0) {
      continue;
    }

    int minutos_apagada = obtener_minutos_apagada((*estado_maquina), h);
    std::cout << "minutos_apagada: " << minutos_apagada << std::endl;

    string tipo_arranque;
    double potencia_subida; // MW/min según el tipo de arranque
    if (minutos_apagada >= 240) {
      tipo_arranque = "frio";
      potencia_subida = 1.5;
    } else if (minutos_apagada >= 120) {
      tipo_arranque = "tibio";
      potencia_subida = 2.6;
    } else {
      tipo_arranque = "caliente";
      potencia_subida = 11.0;
    }

    // Tiempo en minutos para alcanzar la dßemanda (si parte de 0)
    double tiempo_subida = demanda[h].valor / potencia_subida; // en minutos
    int horas_subida = static_cast<int>(ceil(tiempo_subida / 60.0));

    // Marca las horas anteriores como encendidas si es necesario
    for (int k = 0; k < horas_subida; ++k) {
        int hora_encender = h - k;
        if (hora_encender >= 0) {
            (*encender)[hora_encender] = true;
            (*estado_maquina)[hora_encender] = true;
        }
    }

    // Costo de operación por hora
    double consumo_gn_m3 = demanda[h].valor * CONSUMO_GN_M3PKWH;
    double costo_operacion = consumo_gn_m3 * COSTO_GN_USD_M3;

    if ((h == 0 && (*estado_maquina)[h]) || ((*estado_maquina)[h] && (h == horas - 1 || !(*estado_maquina)[h + 1]))) {
        costo_operacion += COSTO_ARRANQUE;
    }

    // Lógica simple: si hay demanda, prendemos la máquina y aceptamos el costo
    (*encender)[h] = true;
    (*estado_maquina)[h] = true;

    // (Podrías comparar contra un costo alternativo o hacer optimización aquí)
    cout << "Hora " << h << "\n"
         << "Demanda = " << demanda[h].valor << " kWh\n"
         << "Arranque = " << tipo_arranque << "\n"
         << "Costo operacion = " << costo_operacion << " USD\n"
         << "encender: " << (*encender)[h] << std::endl;
  }
  int i = 0;
  for (const auto &en : *encender) {
    std::cout << "Hora " << i <<(en ? " 1" : " 0") << ", \n";
    i++;
  }
  return encender;
}
