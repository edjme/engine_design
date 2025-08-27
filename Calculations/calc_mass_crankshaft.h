#ifndef CALC_MASS_CRANKSHAFT_H
#define CALC_MASS_CRANKSHAFT_H

#include "input_data/input.h"
#include <string>

struct CrankshaftMassResults {
    double total_mass;     // кг — масса одного колена
    double x_cg;           // м — расстояние ЦТ от оси коренной шейки (по X)
    double m_root_reduce;  // кг/м^2 — приведённая масса колена к оси коренной
    double m_rotating;     // кг/м^2 — масса вращающихся частей (m_root_reduce + m2)
    const double axis_p; //p.length_root_neck + 2.0*p.depth_web + p.length_rod_neck;    (расстояние между осями цилиндров для полноопорного)
    const double web_p;  //p.length_root_neck + 3.0*p.depth_web + 2.0*p.length_rod_neck; (расстояние между щеками для полноопорного)
    const double axis_n; //p.length_rod_neck  + p.depth_web;        (расстояние между осями цилиндров для неполноопорного)
    const double web_n;  //2.0*p.depth_web    + 2.0*p.length_rod_neck; (расстояние между щеками для неполноопорного)
};

// Расчёт массы и ЦТ (в метрах/килограммах)
CrankshaftMassResults calc_mass_crankshaft(const Params& p);

// Экспорт 3D-модели колена вала в STL (ASCII), КООРДИНАТЫ В МИЛЛИМЕТРАХ
// seg — дискретизация окружностей
bool export_crank_STL_mm(const Params& p,
                         const std::string& stl_path,
                         int seg = 128);

#endif // CALC_MASS_CRANKSHAFT_H
