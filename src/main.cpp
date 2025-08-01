#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include "../include/calculador_costos_maquina.hpp"

using namespace std;

// Constantes energéticas
const double POT_GAS1 = 171;
const double POT_GAS2 = 171+171;
const double POT_VAPOR = 563;
const double CONSUMO_GN_M3PKWH = 0.167 / 0.717;
const double COSTO_GN_USD_M3 = 618;
const double COSTO_AGUA = std::numeric_limits<double>::max();
const double COSTO_ARRANQUE = 4208.0 + (5.110 * COSTO_GN_USD_M3);

vector<double> obtener_demanda()
{
    std::string ruta = "../data/parametros.in";
    std::ifstream archivo(ruta);
    std::vector<double> datos;
    std::string linea;
    std::getline(archivo, linea);

    while (std::getline(archivo, linea))
    {

        std::stringstream ss(linea);
        std::string hora, valor_str;
        std::getline(ss, hora, ',');
        std::getline(ss, valor_str, ',');

        double demanda;
        demanda = std::stod(valor_str);
        datos.push_back(demanda);
    }

    return datos;
}

std::vector<double> generar_demanda_aleatoria()
{
    std::vector<double> datos;
    std::srand(static_cast<unsigned int>(std::time(nullptr))); // Semilla aleatoria
    for (int dias = 0; dias < 7; ++dias)
    {
        for (int h = 0; h < 24; ++h)
        {
            double d;
            d = static_cast<double>(std::rand() % 2201); // [0, 2200]
            datos.push_back(d);
        }
    }

    return datos;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::vector<double> demanda = obtener_demanda();

    if (size < 4)
    {
        if (rank == 0)
            std::cerr << "Se requieren al menos 4 procesos.\n";
        MPI_Finalize();
        return 1;
    }

    std::vector<RespuestaMaquina> *encender = new std::vector<RespuestaMaquina>(demanda.size(), {0, 0});

    for (int eo = 0; eo <= 1464; ++eo)
    {
        std::string tipo_maquina;
        int horas_apagada = 0;
        double costo_operacion = 0.0;
        for (int h = 0; h < demanda.size(); ++h)
        {
            double demanda_h = demanda[h];
            double eolica = 0.0;
            if (rank == 0)
            {
                std::cout << "\nHora " << h
                          << " - Demanda original: " << demanda_h
                          << " kWh | Eólica disponible: " << eolica << " kWh" << std::endl;
            }
            MPI_Bcast(&eolica, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            demanda_h -= eolica;
            if (demanda_h < 0)
                demanda_h = 0;

            MPI_Bcast(&demanda_h, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            RespuestaMaquina respuesta;
            respuesta.costo = 0;
            respuesta.encendida = 0;

            double potencia_disponible = 0;
            if (rank == 0)
                potencia_disponible = POT_GAS1;
            else if (rank == 1)
                potencia_disponible = POT_GAS2;
            else if (rank == 2)
                potencia_disponible = POT_VAPOR + POT_GAS1 + POT_GAS1;
            else if (rank == 3)
                potencia_disponible = std::numeric_limits<double>::max();

            // Aca le paso a calculador_costos y tengo que cambiarlo para que reciba la potencia, pero despues haga todos los mismos calculos
            // Complejizarlo para que tenga una justificacion la division de las maquinas

            if (demanda_h <= 0.0)
            {
                horas_apagada++;
                std::cout << "Horas apagadas:" + std::to_string(horas_apagada) << std::endl;

                continue;
            }
            else
            {
                respuesta = calcular_costo(eo, demanda_h,h, potencia_disponible, horas_apagada);
            }

            std::vector<RespuestaMaquina> respuestas(size);
            MPI_Gather(&respuesta, sizeof(RespuestaMaquina), MPI_BYTE,
                       respuestas.data(), sizeof(RespuestaMaquina), MPI_BYTE,
                       0, MPI_COMM_WORLD);

            if (rank == 0)
            {
                double costo_total = 0;
                for (int i = 0; i < size; ++i)
                {
                    std::cout << "Proceso " << i
                              << " kWh | Costo: " << respuestas[i].costo << " USD"
                              << (respuestas[i].encendida ? " [ON]\n" : " [OFF]\n");
                    costo_total += respuestas[i].costo;
                }
                std::cout << " | Costo total: " << costo_total << " USD\n";
            } // Aca tengo que ver cual fue de menor costo y quedarme con esa
        }
    }

    MPI_Finalize();
    return 0;
}