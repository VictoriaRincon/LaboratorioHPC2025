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
const double POT_GAS2 = 171 + 171;
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
    for (int dias = 0; dias < 1; ++dias)
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
    std::ofstream archivo_resultado;
    long costo_total_operacion = 0.0;
    if (rank == 0)
    {
        archivo_resultado.open("seleccion_maquinas.csv");
        archivo_resultado << "Hora,MaquinaSeleccionada,Costo,Encendida\n";
    }

    // std::vector<RespuestaMaquina> *encender = new std::vector<RespuestaMaquina>(demanda.size(), {0, 0});

    for (int eolica = 1494; eolica <= 1494; ++eolica) // Maxima eolica 1464
    {
        std::string tipo_maquina;
        int horas_apagada = 0;
        double costo_operacion = 0.0;
        for (int h = 0; h < demanda.size(); ++h)
        {
            double demanda_h = demanda[h];
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

            if (demanda_h <= 0.0)
            {
                horas_apagada++;
                std::cout << "Horas apagadas:" + std::to_string(horas_apagada) << std::endl;

                continue;
            }
            else
            {
                respuesta = calcular_costo(eolica, demanda_h, h, potencia_disponible, horas_apagada);
                horas_apagada = 0; // Reseteamos horas apagada
            }

            std::vector<RespuestaMaquina> respuestas(size);
            MPI_Gather(&respuesta, sizeof(RespuestaMaquina), MPI_BYTE,
                       respuestas.data(), sizeof(RespuestaMaquina), MPI_BYTE,
                       0, MPI_COMM_WORLD);

            if (rank == 0)
            {
                double costo_total = 0;
                int mejor_proceso = -1;
                double horas_encendida = 0;
                double menor_costo = std::numeric_limits<double>::max();

                for (int i = 0; i < size; ++i)
                {
                    std::cout << "Proceso " << i
                              << " kWh | Costo: " << respuestas[i].costo << " USD"
                              << (respuestas[i].encendida ? " [ON]\n" : " [OFF]\n");
                    // Obtengo el de menor costo
                    if (respuestas[i].costo < menor_costo && respuestas[i].encendida > 0)
                    {
                        menor_costo = respuestas[i].costo;
                        mejor_proceso = i;
                        horas_encendida = respuestas[i].encendida;
                    }
                }

                if (mejor_proceso != -1)
                {
                    costo_total += menor_costo;
                    std::cout << " | Costo total: " << costo_total << " USD\n";
                    archivo_resultado << h << "," << mejor_proceso << "," << menor_costo << "," << horas_encendida << "\n";
                }
                costo_total_operacion += costo_total;
            }
        }
    }
    if (rank == 0)
        archivo_resultado << "Costo total: " << costo_total_operacion << " USD\n";

    MPI_Finalize();
    return 0;
}