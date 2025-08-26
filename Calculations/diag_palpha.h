#ifndef DIAG_PALPHA_H
#define DIAG_PALPHA_H

#include <vector>
#include <string>
#include "input_data/input.h"
#include "Calculations/ind_diagr.h" // тут объявлен IndicatorResults

struct PAlphaResults {
    // Сетка
    std::vector<double> alpha_deg; // 0..720, град
    std::vector<double> V_alpha;   // м^3
    std::vector<double> P_alpha;   // Па

    // Найденный угол изобарного участка (добавление теплоты)
    double phi_deg;  // град

    // Диагностика
    std::string summary;
};

// Построение P(α) с шагом step_deg (например, 0.5 или 1.0 град).
// Если заданы пути — сохранит CSV и HTML-график; auto_open_html=true — откроет HTML (Windows).
PAlphaResults build_P_alpha(const Params& p,
                            const IndicatorResults& ind,
                            double step_deg = 0.5,
                            const std::string& output_csv_path  = "",
                            const std::string& output_html_path = "",
                            bool auto_open_html = false);

#endif // DIAG_PALPHA_H