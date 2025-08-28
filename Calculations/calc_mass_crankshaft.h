// Calculations/calc_mass_crankshaft.h
#ifndef CALC_MASS_CRANKSHAFT_H
#define CALC_MASS_CRANKSHAFT_H

#include "input_data/input.h"
#include <string>

struct CrankshaftMassResults {
    double total_mass;    // кг
    double x_cg;          // м
    double m_root_reduce; // кг/м^2
    double m_rotating;    // кг/м^2
    double axis_p;        // контрольные расстояния (для CW)
    double web_p;
    double axis_n;
    double web_n;
};

// Расчёт масс и ЦТ (метры/килограммы)
CrankshaftMassResults calc_mass_crankshaft(const Params& p);

// Экспорт 3D-модели колена вала в STL (мм)
bool export_crank_STL_mm(const Params& p,
                         const std::string& stl_path,
                         int seg = 128);

#endif // CALC_MASS_CRANKSHAFT_H
