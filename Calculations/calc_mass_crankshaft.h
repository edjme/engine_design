// Calculations/calc_mass_crankshaft.h
#ifndef CALC_MASS_CRANKSHAFT_H
#define CALC_MASS_CRANKSHAFT_H

#include "input_data/input.h"
#include <string>

// Итоги расчёта масс/СТ (в СИ: метры, килограммы).
// m_root_reduce и m_rotating — ПРИВЕДЁННЫЕ массы к оси коренной (кг/м^2).
struct CrankshaftMassResults {
    double total_mass;     // кг
    double x_cg;           // м (от оси коренной шейки по оси X к шатунной)
    double m_root_reduce;  // кг/м^2
    double m_rotating;     // кг/м^2

    // Контрольные расстояния по оси Z для сборки (удобно для CW/геометрии)
    double axis_p;         // полноопорный: Lm + 2*Wz + Lr
    double web_p;          // полноопорный: Lm + 3*Wz + 2*Lr
    double axis_n;         // неполноопорный: Lr + Wz
    double web_n;          // неполноопорный: 2*Wz + 2*Lr
};

// Расчёт масс и СТ (метры/килограммы)
CrankshaftMassResults calc_mass_crankshaft(const Params& p);

// Экспорт 3D-модели колена в STL (мм)
bool export_crank_STL_mm(const Params& p,
                         const std::string& stl_path,
                         int seg = 128);

#endif // CALC_MASS_CRANKSHAFT_H
