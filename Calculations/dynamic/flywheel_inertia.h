#pragma once
// Calculations/flywheel_inertia.h

#include <string>
#include <vector>

#include "input_data/input.h"                // Params
#include "Calculations/dynamic/forces_ksm.h" // ForcesResults

struct FlywheelInertiaResults
{
    // базовые
    double I_kv_cw{0.0};
    int cylinders{1};
    double delta{0.0}; // использованная δ

    // массы
    double Fp{0.0};
    double M2{0.0};
    double M_eq{0.0};
    double I_mm_one{0.0};

    // энергетика
    double Ms{0.0};
    double dA{0.0};
    double I0_needed{0.0};
    double I_fly{0.0};

    // для графиков/CSV
    std::vector<double> alpha_deg; // α
    std::vector<double> M;         // крутящий момент
    std::vector<double> MminusMs;  // M - Ms
    std::vector<double> A_cum;     // накопленная избыточная работа

    std::string summary;
};

// Считает инерции, строит CSV/HTML-графики.
// Если delta<=0 → берём p.delta (добавь это поле в Params).
FlywheelInertiaResults compute_flywheel_inertia(
    const Params &p,
    const ForcesResults &fr,
    double I_kv_cw,              // момент инерции «колено+CW» об оси коренной
    int cylinders,               // число цилиндров
    double delta,                // можно передать <=0 чтобы читать из p.delta
    const std::string &out_csv,  // путь CSV (можно пустую строку)
    const std::string &out_html, // путь HTML (можно пустую строку)
    bool auto_open_html          // открыть HTML (Windows)
);
