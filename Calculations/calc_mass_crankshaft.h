#ifndef CALC_MASS_CRANKSHAFT_H
#define CALC_MASS_CRANKSHAFT_H

#include <string>
#include "input_data/input.h"

// Оси: Y — высота (от коренной к шатунной), Z — вдоль коренной (ось коренной),
// X — ширина.
struct CrankshaftMassResults {
    double total_mass    = 0.0; // кг
    double y_cg          = 0.0; // м, расстояние ЦТ от оси коренной по Y (вверх)
    double m_root_reduce = 0.0; // кг/м^2
    double m_rotating    = 0.0; // кг/м^2

    // Контрольные расстояния вдоль Z
    double axis_p = 0.0; // L_root + 2*depth_web + L_rod
    double web_p  = 0.0; // L_root + 3*depth_web + 2*L_rod
    double axis_n = 0.0; // L_rod + depth_web
    double web_n  = 0.0; // 2*depth_web + 2*L_rod
};

// Масса/ЦТ (с вычитанием объёмов галтелей) и контрольные расстояния.
CrankshaftMassResults calc_mass_crankshaft(const Params& p);

// Экспорт STL (мм) в системе Y↑, Z→ (ось коренной), X — ширина.
// В STL галтели визуально не вырезаются (без CSG), но в массе учтены.
bool export_crank_STL_mm(const Params& p, const std::string& stl_path, int seg = 128);

#endif // CALC_MASS_CRANKSHAFT_H
