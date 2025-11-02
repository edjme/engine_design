#ifndef CALC_PALPHA_H
#define CALC_PALPHA_H

#include <vector>
#include <string>
#include "../input_data/Palpha_input.h"

// ------------------ Структура результатов ------------------
struct PAlphaResults
{
    std::vector<double> alpha_deg; // углы α, град
    std::vector<double> P_alpha;   // давления, Па
    std::string htmlContent;       // готовый HTML для вывода
    std::string summary;           // текстовая сводка
};

// ------------------ Основная функция ------------------
PAlphaResults build_P_alpha(const UnwrapParams &params);

#endif // DIAG_PALPHA_H
