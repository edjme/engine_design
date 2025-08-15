#ifndef CALC_DYNAMIC_H
#define CALC_DYNAMIC_H

#include <vector>
#include <string>
#include "input_data/input.h"

// Результаты индикаторной диаграммы
struct IndicatorResults {
    double F_p;
    double Vh, Vc, Va;
    double Pc, Pz, Pz_, Pb;
    double Pa, Pr;
    double Vz, Vz_;

    std::vector<double> V_path;
    std::vector<double> P_path;

    std::vector<double> V_comp,    P_comp;
    std::vector<double> V_iso_add, P_iso_add;
    std::vector<double> V_preexp,  P_preexp;
    std::vector<double> V_exp,     P_exp;
    std::vector<double> V_drop_b,  P_drop_b;
    std::vector<double> V_exh,     P_exh;
    std::vector<double> V_drop_r,  P_drop_r;
    std::vector<double> V_int,     P_int;

    double A_cycle;
    double Q_in;
    double eta;

    std::string summary;
};

// Построение индикаторной диаграммы (P–V).
// Если указать пути, сохранит CSV и HTML (интерактивный просмотр в браузере).
// auto_open=true — попробует автоматически открыть HTML (Windows).
IndicatorResults build_indicator_PV(const Params& p,
                                    int samples_per_segment = 400,
                                    const std::string& output_csv_path = "",
                                    const std::string& output_html_path = "",
                                    bool auto_open_html = false);

#endif // CALC_DYNAMIC_H
