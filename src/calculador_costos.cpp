#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const double BASE_COSTO_ARRANQUE = 4208.0;
const double MULT_COSTO_GN = 5.110;
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
const double POTENCIA_MINIMA = 14; // MW/min

struct Demanda
{
  double hora;
  double valor;
};

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

std::vector<Demanda> generar_demanda_aleatoria()
{
  std::vector<Demanda> datos;
  std::srand(static_cast<unsigned int>(std::time(nullptr))); // Semilla aleatoria
  for (int dias = 0; dias < 1; ++dias)
  {
    for (int h = 0; h < 24; ++h)
    {
      Demanda d;
      d.hora = h;
      d.valor = static_cast<double>(std::rand() % 2201); // [0, 2200]
      datos.push_back(d);
    }
  }

  return datos;
}

vector<bool> *calcular_costo(double esc)
{
  vector<Demanda> demanda = obtener_demanda(); // Puedo leer un archivo o que sea un arreglo de 24
  // vector<Demanda> demanda = generar_demanda_aleatoria();

  int horas = demanda.size();
  std::vector<bool> *encender = new std::vector<bool>(horas, false);

  // Simulación hacia atrás
  double demanda_sin_abastecer = 0.0;
  std::string tipo_maquina;
  int horas_apagada = 0;
  double costo_operacion = 0.0;

  for (int h = 0; h < horas; ++h)
  {
    std::cout << "Hora:" + std::to_string(h) << std::endl;
    std::cout << "Demanda:" + std::to_string(demanda[h].valor) << std::endl;

    demanda_sin_abastecer = demanda[h].valor - esc;
    std::cout << "Demanda a complacer:" + std::to_string(demanda_sin_abastecer) << std::endl;

    if (demanda_sin_abastecer <= 0.0)
    {
      horas_apagada++;
      continue;
    }

    // Si no conviene mantener o no hay encendido futuro, prender normal
    (*encender)[h] = true;

    // Asignar tipo_maquina según el valor de demanda
    if (demanda_sin_abastecer <= 171.0)
    {
      tipo_maquina = "gas";
    }
    else if (demanda_sin_abastecer <= (171.0 * 2))
    {
      tipo_maquina = "ambas maquinas de gas";
    }
    else if (demanda_sin_abastecer <= POTENCIA_MAXIMA_MCC)
    {
      tipo_maquina = "medio ciclo combinado";
    }
    else if (demanda_sin_abastecer <= POTENCIA_MAXIMA_CC)
    {
      tipo_maquina = "ciclo combinado completo";
    }
    else if (demanda_sin_abastecer > 905.0)
    {
      tipo_maquina = "agua + ciclo combinado completo";
    }

    // Determinar tipo de arranque según horas_apagada
    double tipo_arranque = 1;
    if (horas_apagada != 0 && horas_apagada <= 2)
      tipo_arranque = 0.2;
    else if (horas_apagada > 2)
      tipo_arranque = 0.5;

    std::cout << "Horas apagadas:" + std::to_string(horas_apagada) << std::endl;

    if (horas_apagada > 0)
    {
      double costo_mantener = POTENCIA_MINIMA * CONSUMO_GN_M3PKWH * COSTO_GN_USD_M3 * horas_apagada;

      if (costo_mantener < COSTO_ARRANQUE)
      {
        int k = h - 1;
        while (k >= 0 && !(*encender)[k])
        {
          (*encender)[k] = true;
          k--;
        }
        std::cout << "Entre h=" << h << " y h=" << h - horas_apagada
                  << " conviene mantener prendida (costo mantener: "
                  << costo_mantener << " < arranque: "
                  << COSTO_ARRANQUE << ")\n";
        horas_apagada = 0; // Reseteamos horas apagada
        continue;          // ya está todo marcado, salteamos lógica de encendido
      }
      else
      {
        std::cout << "Entre h=" << h << " y h=" << h - horas_apagada
                  << " conviene mantener apagada (costo mantener: "
                  << costo_mantener << " > arranque: "
                  << COSTO_ARRANQUE << ")\n";
      }
    }

    // Costo de operación por hora
    double consumo_gn_m3 = demanda_sin_abastecer * CONSUMO_GN_M3PKWH;
    costo_operacion += consumo_gn_m3 * COSTO_GN_USD_M3;

    // Tiempo en minutos para alcanzar la demanda (si parte de 0)
    double tiempo_subida = demanda_sin_abastecer / POTENCIA_SUBIDA; // en minutos
    int horas_subida = static_cast<int>(ceil(tiempo_subida / 60.0));

    if ((h == 0 && (*encender)[h]) || (!(*encender)[h - 1]))
    {
      costo_operacion += (COSTO_ARRANQUE * tipo_arranque);
    }

    // Lógica simple: si hay demanda, prendemos la máquina y aceptamos el costo
    (*encender)[h] = true;

    // (Podrías comparar contra un costo alternativo o hacer optimización aquí)
    cout << "Demanda = " << demanda_sin_abastecer << " kWh\n"
         << "Arranque = " << tipo_arranque << "\n"
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
