#ifndef ESCENARIO_HPP
#define ESCENARIO_HPP

struct Escenario {
    double hora;
    double energia_eolica;
};

double escenario(const Escenario& esc);

#endif