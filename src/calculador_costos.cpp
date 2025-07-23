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
const double DENSIDAD_GN = 0.717; // kg/m3 (valor aproximado)
const double CONSUMO_GN_GPKWH =  167; // gramos de GN por kWh
const double CONSUMO_GN_KGPKWH = CONSUMO_GN_GPKWH / 1000.0; // g/kWh a kg/kWh
const double CONSUMO_GN_M3PKWH = CONSUMO_GN_KGPKWH / DENSIDAD_GN;

struct Demanda {
  std::string hora;
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

double calcular_costo_arranque(double costo_gn_usd_m3) {
  return BASE_COSTO_ARRANQUE + MULT_COSTO_GN * costo_gn_usd_m3;
}

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
    demanda.hora = hora;
    demanda.valor = std::stod(valor_str);
    datos.push_back(demanda);
  }

  return datos;
}

vector<bool> calcular_costo(const Escenario &esc, double costo_gn_usd_m3
) {

  vector<Demanda> demanda = obtener_demanda(); // Puedo leer un archivo o que sea un arreglo de 24
    //for (const auto &dem : demanda) {
    //bool necesidadEncender = (demanda.end()->valor - esc.energia_eolica) > 0.0;
    //double costoTotal = 0.0;
    
    int horas = demanda.size();
    vector<bool> encender(horas, false); // resultado: prender o no la máquina
    vector<bool> estado_maquina(horas, false); // indica si estuvo prendida

  // Simulación hacia atrás
  for (int h = horas - 1; h >= 0; --h) {
    if (demanda[h].valor <= 0) {
      estado_maquina[h] = false;
      continue;
    }

    int minutos_apagada = obtener_minutos_apagada(estado_maquina, h);

    string tipo_arranque;
    if (minutos_apagada >= 240) {
      tipo_arranque = "frio";
    } else if (minutos_apagada >= 120) {
      tipo_arranque = "tibio";
    } else {
      tipo_arranque = "caliente";
    }

    // Costo de arranque (solo si estuvo apagada)
    double costo_arranque = (minutos_apagada > 0) ? calcular_costo_arranque(costo_gn_usd_m3) : 0.0;

    // Costo de operación por hora
    double consumo_gn_m3 = demanda[h].valor * CONSUMO_GN_M3PKWH;
    double costo_operacion = consumo_gn_m3 * costo_gn_usd_m3;

    // Lógica simple: si hay demanda, prendemos la máquina y aceptamos el costo
    encender[h] = true;
    estado_maquina[h] = true;

    // (Podrías comparar contra un costo alternativo o hacer optimización aquí)
    cout << "Hora " << h << ": "
         << "Demanda = " << demanda[h].valor << " kWh, "
         << "Arranque = " << tipo_arranque << ", "
         << "Costo arranque = " << costo_arranque << " USD, "
         << "Costo operacion = " << costo_operacion << " USD\n";
  }

  return encender;
}
