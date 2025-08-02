#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../include/calculador_costos_maquina.hpp"

using namespace std;

const double BASE_COSTO_ARRANQUE = 4208.0;
const double MULT_COSTO_GN = 51.10;
const double DENSIDAD_GN = 0.717;                           // kg/m3 (valor aproximado)
const double CONSUMO_GN_GPKWH = 167;                        // gramos de GN por kWh
const double CONSUMO_GN_KGPKWH = CONSUMO_GN_GPKWH / 1000.0; // g/kWh a kg/kWh
const double CONSUMO_GN_M3PKWH = CONSUMO_GN_KGPKWH / DENSIDAD_GN;
const double COSTO_GN_USD_M3 = 618;
const double COSTO_ARRANQUE = BASE_COSTO_ARRANQUE + (MULT_COSTO_GN * COSTO_GN_USD_M3);
const double POTENCIA_MAXIMA_GAS = 171;
const double POTENCIA_MAXIMA_MCC = POTENCIA_MAXIMA_GAS + 563.0;
const double POTENCIA_MAXIMA_CC = (POTENCIA_MAXIMA_GAS * 2) + 563.0;
const double POTENCIA_SUBIDA = 15; // MW/min según el tipo de arranque
const double POTENCIA_MINIMA = 14; // MW/min que varie por cada maquina

RespuestaMaquina calcular_costo(double esc, double demanda_h, double hora, double pot_disponible, double horas_apagada)
{

  RespuestaMaquina respuesta;
  respuesta.costo = 0;
  respuesta.encendida = 0;

  // Determinar tipo de arranque según horas_apagada
  double tipo_arranque = 1;
  if (horas_apagada != 0 && horas_apagada <= 2)
    tipo_arranque = 0.2;
  else if (horas_apagada > 2)
    tipo_arranque = 0.5;

  respuesta.encendida = 1;
  if (horas_apagada > 0)
  {
    double costo_mantener = POTENCIA_MINIMA * CONSUMO_GN_M3PKWH * COSTO_GN_USD_M3 * horas_apagada;

    if (costo_mantener < COSTO_ARRANQUE)
    {
      std::cout << "Entre h=" << hora << " y h=" << hora - horas_apagada
                << " conviene mantener prendida (costo mantener: "
                << costo_mantener << " < arranque: "
                << COSTO_ARRANQUE << ")\n";
      respuesta.encendida += horas_apagada; // Marca que la máquina estuvo encendida
      respuesta.costo += costo_mantener;
    }
    else
    {
      std::cout << "Entre h=" << hora << " y h=" << hora - horas_apagada
                << " conviene mantener apagada (costo mantener: "
                << costo_mantener << " > arranque: "
                << COSTO_ARRANQUE << ")\n";
      respuesta.costo += (COSTO_ARRANQUE * tipo_arranque);
    }
  }

  if (hora == 0) respuesta.costo += COSTO_ARRANQUE;
  
  // Costo de operación por hora
  double consumo_gn_m3 = demanda_h * CONSUMO_GN_M3PKWH;
  respuesta.costo += consumo_gn_m3 * COSTO_GN_USD_M3;

  // Tiempo en minutos para alcanzar la demanda (si parte de 0)
  double tiempo_subida = demanda_h / POTENCIA_SUBIDA; // en minutos
  int horas_subida = static_cast<int>(ceil(tiempo_subida / 60.0));

  if(respuesta.encendida < horas_subida)
  {
    respuesta.encendida += horas_subida; // Marca que la máquina estuvo encendida
  }

  return respuesta;
}
