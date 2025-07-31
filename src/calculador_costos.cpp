#include "../include/escenario.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

enum EstadoMaquina
{
  APAGADA,
  PRENDIDA
};

const double BASE_COSTO_ARRANQUE = 420.80;
const double MULT_COSTO_GN = 51.10;
const double DENSIDAD_GN = 0.717;                           // kg/m3 (valor aproximado)
const double CONSUMO_GN_GPKWH = 167;                        // gramos de GN por kWh
const double CONSUMO_GN_KGPKWH = CONSUMO_GN_GPKWH / 1000.0; // g/kWh a kg/kWh
const double CONSUMO_GN_M3PKWH = CONSUMO_GN_KGPKWH / DENSIDAD_GN;
const double COSTO_GN_USD_M3 = 618;
const double COSTO_ARRANQUE =
    BASE_COSTO_ARRANQUE + MULT_COSTO_GN * COSTO_GN_USD_M3;
const double POTENCIA_MAXIMA_GAS = 171;
const double POTENCIA_MAXIMA_MCC = POTENCIA_MAXIMA_GAS + 563.0;
const double POTENCIA_MAXIMA_CC = (POTENCIA_MAXIMA_GAS * 2) + 563.0;

struct Demanda
{
  double hora;
  double valor;
};

// Función que determina el tipo de arranque
int obtener_minutos_apagada(const vector<bool> &encendida, int hora_actual)
{
  int minutos = 0;
  for (int i = hora_actual + 1; i < encendida.size(); ++i)
  {
    if (encendida[i])
      break;
    minutos += 60;
  }
  return minutos;
}

double costo_mantener_prendida(const vector<Demanda> &demanda, int inicio, int fin)
{
  double costo = 0.0;
  for (int h = inicio; h < fin; ++h)
  {
    costo += demanda[h].valor * CONSUMO_GN_M3PKWH * COSTO_GN_USD_M3;
  }
  return costo;
}

vector<Demanda> obtener_demanda()
{
  std::string ruta = "../data/parametros.in";
  std::ifstream archivo(ruta);
  std::vector<Demanda> datos;
  std::string linea;
  std::getline(archivo, linea);

  while (std::getline(archivo, linea))
  {

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

vector<bool> *calcular_costo(double esc)
{
  vector<Demanda> demanda = obtener_demanda(); // Puedo leer un archivo o que sea un arreglo de 24

  int horas = demanda.size();
  std::vector<bool> *encender = new std::vector<bool>(horas, false);
  std::vector<bool> *estado_maquina = new std::vector<bool>(horas, false);

  // Simulación hacia atrás
  double demandaAComplacer = 0.0;
  std::string tipo_maquina;

  for (int h = horas - 1; h >= 0; --h)
  {
    std::cout << "Hora:" + std::to_string(h) << std::endl;
    std::cout << "Demanda:" + std::to_string(demanda[h].valor) << std::endl;

    demandaAComplacer = demanda[h].valor - esc;

    if (demandaAComplacer <= 0.0)
    {
      continue;
    }

    // Asignar tipo_maquina según el valor de demanda
    if (demandaAComplacer<= 171.0) {
        tipo_maquina = "gas";
    } else if (demandaAComplacer <= (171.0 * 2)) {
        tipo_maquina = "ambas maquinas de gas";
    } else if (demandaAComplacer <= POTENCIA_MAXIMA_MCC) {
        tipo_maquina = "medio ciclo combinado";
    } else if (demandaAComplacer <= POTENCIA_MAXIMA_CC) {
        tipo_maquina = "ciclo combinado completo";
    } else if (demandaAComplacer > 905.0) {
        tipo_maquina = "agua";
    }

    // Buscar si ya se prendió en el futuro
    int h_proxima = h + 1;
    while (h_proxima < horas && !(*estado_maquina)[h_proxima]) {
        h_proxima++;
    }

    if (h_proxima < horas) {
        // Ya está programado un encendido futuro
        double costo_mantener = costo_mantener_prendida(demanda, h, h_proxima);
        double costo_apagar_y_prender = COSTO_ARRANQUE;

        if (costo_mantener < costo_apagar_y_prender) {
            for (int k = h; k < h_proxima; ++k) {
                (*encender)[k] = true;
                (*estado_maquina)[k] = true;
            }
            continue; // ya está todo marcado, salteamos lógica de encendido
            std::cout << "Entre h=" << h << " y h=" << h_proxima
                      << " conviene mantener prendida (costo mantener: "
                      << costo_mantener << " < arranque: "
                      << COSTO_ARRANQUE << ")\n";
        }else{
                      std::cout << "Entre h=" << h << " y h=" << h_proxima
                      << " conviene mantener apagada (costo mantener: "
                      << costo_mantener << " > arranque: "
                      << COSTO_ARRANQUE << ")\n";
        }
    }

    // Si no conviene mantener o no hay encendido futuro, prender normal
    (*encender)[h] = true;
    (*estado_maquina)[h] = true;

    double potencia_subida = 15; // MW/min según el tipo de arranque

    // Tiempo en minutos para alcanzar la demanda (si parte de 0)
    double tiempo_subida = demandaAComplacer / potencia_subida; // en minutos
    int horas_subida = static_cast<int>(ceil(tiempo_subida / 60.0));

    // Marca las horas anteriores como encendidas si es necesario
    for (int k = 0; k < horas_subida; ++k)
    {
      int hora_encender = h - k;
      if (hora_encender >= 0)
      {
        (*encender)[hora_encender] = true;
        (*estado_maquina)[hora_encender] = true;
      }
    }

    // Costo de operación por hora
    double consumo_gn_m3 = demanda[h].valor * CONSUMO_GN_M3PKWH;
    double costo_operacion = consumo_gn_m3 * COSTO_GN_USD_M3;

    if ((h == 0 && (*estado_maquina)[h]) || (((*estado_maquina)[h] && !(*estado_maquina)[h - 1])))
    {
      costo_operacion += COSTO_ARRANQUE;
    }

    // Lógica simple: si hay demanda, prendemos la máquina y aceptamos el costo
    (*encender)[h] = true;
    (*estado_maquina)[h] = true;

    // (Podrías comparar contra un costo alternativo o hacer optimización aquí)
    cout << "Hora " << h << "\n"
         << "Demanda = " << demandaAComplacer << " kWh\n"
         //<< "Arranque = " << tipo_arranque << "\n"
         << "Costo operacion = " << costo_operacion << " USD\n"
         << "encender: " << (*encender)[h] << "\n"
         << "Tipo de máquina utilizada: " << tipo_maquina << std::endl;

  }
  int i = 0;
  for (const auto &en : *encender)
  {
    std::cout << "Hora " << i << (en ? " 1" : " 0") << ", \n";
    i++;
  }
  return encender;
}
