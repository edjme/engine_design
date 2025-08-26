#ifndef CALC_MASS_CRANKSHAFT_H
#define CALC_MASS_CRANKSHAFT_H

#include "input_data/input.h"
#include <string>

struct CrankshaftMassResults {
    double total_mass;     // кг — масса одного колена (две щеки + коренная + шатунная)
    double x_cg;           // м — расстояние ЦТ от оси коренной шейки (по X)
    double m_root_reduce;  // кг/м^2 — приведённая масса колена к оси коренной
    double m_rotating;     // кг/м^2 — масса вращающихся частей (m_root_reduce + m2)
};

// Расчёт массы и ЦТ (в метрах/килограммах)
CrankshaftMassResults calc_mass_crankshaft(const Params& p);

// Экспорт упрощённой 3D-модели колена вала в STL (ASCII), КООРДИНАТЫ В МИЛЛИМЕТРАХ
// seg — дискретизация окружностей
bool export_crank_STL_mm(const Params& p,
                         const std::string& stl_path,
                         int seg = 128);

#endif // CALC_MASS_CRANKSHAFT_H
