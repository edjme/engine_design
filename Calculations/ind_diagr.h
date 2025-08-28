#ifndef IND_DIAGR_H
#define IND_DIAGR_H

#include <vector>
#include <string>
#include "input_data/input.h"

// Результаты построения индикаторной диаграммы P–V.
// Единицы: V — м^3, P — Па, работы/теплоты — Дж.
struct IndicatorResults {
    // Базовые величины
    double F_p = 0.0;   // площадь поршня, м^2
    double Vh  = 0.0;   // рабочий объём, м^3
    double Vc  = 0.0;   // объём камеры, м^3
    double Va  = 0.0;   // объём в нижней мёртвой точке, м^3

    // Давления опорных точек (Па)
    double Pa  = 0.0;   // впуск
    double Pr  = 0.0;   // выпуск
    double Pc  = 0.0;   // конец сжатия
    double Pz  = 0.0;   // пик/уровень изобары
    double Pz_ = 0.0;   // изобарный участок (обычно = Pz)
    double Pb  = 0.0;   // конец расширения

    // Объёмы ключевых точек (м^3)
    double Vz  = 0.0;   // объём в ВМТ
    double Vz_ = 0.0;   // объём после изобарного подогрева

    // Участки для визуализации (гладкие куски)
    std::vector<double> V_comp,    P_comp;     // a -> c (сжатие)
    std::vector<double> V_iso_add, P_iso_add;  // c -> z (изохора)
    std::vector<double> V_preexp,  P_preexp;   // z -> z' (изобара)
    std::vector<double> V_exp,     P_exp;      // z' -> b (расширение)
    std::vector<double> V_drop_b,  P_drop_b;   // b -> r' (блоудаун)
    std::vector<double> V_exh,     P_exh;      // r' -> r (выпуск, 4Т)
    std::vector<double> V_drop_r,  P_drop_r;   // r -> r'' (вертикаль к Pa)
    std::vector<double> V_int,     P_int;      // r'' -> a (впуск)

    // Итоговый путь (соединение всех участков по выбранной тактности)
    std::vector<double> V_path;
    std::vector<double> P_path;

    // Интегралы (на один цилиндр, за цикл)
    double A_cycle = 0.0; // индикаторная работа, Дж
    double Q_in    = 0.0; // подвод теплоты (по принятой схеме), Дж
    double eta     = 0.0; // КПД индикаторный (A_cycle/Q_in)

    // Диагностика
    std::string summary;
};

// Построение индикаторной диаграммы P–V.
// samples_per_segment: типовое число точек на «гладком» участке (N>=8).
// Если указаны пути — сохраняет CSV ("V,P") и HTML; на Windows auto_open_html может открыть HTML.
IndicatorResults build_indicator_PV(const Params& p,
                                    int samples_per_segment = 400,
                                    const std::string& output_csv_path  = "",
                                    const std::string& output_html_path = "",
                                    bool auto_open_html = false);

#endif // IND_DIAGR_H
