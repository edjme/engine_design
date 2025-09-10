#pragma once
#include <string>
#include "input_data/input.h"
#include "Calculations/calc_mass_crankshaft.h"

// Варианты компоновки противовесов
enum class CWVariant
{
    FullSupport_V1 = 1,
    FullSupport_V2 = 2,
    SemiSupport = 3
};

struct CWResult
{
    bool ok = false;
    double alpha_deg = 0.0; // угол сектора каждого противовеса, град
    double S_prot = 0.0;    // статический момент одного противовеса, кг·м
    double Iz_crank = 0.0;  // I_z тела колена без CW, кг·м^2
    double Iz_cw = 0.0;     // I_z всех противовесов, кг·м^2
    double Iz_total = 0.0;  // I_z(колено+CW), кг·м^2
    std::string message;
};

// Построение колена (сужающиеся щеки как в mass-модуле) + добавление противовесов.
// STL выводится в мм. Сектор противовеса ориентирован вниз по оси X (−X).
CWResult build_counterweights_and_export(
    const Params &p,
    const CrankshaftMassResults &cm,
    CWVariant var,
    const std::string &stl_path_mm,
    int seg = 128,
    bool clamp_alpha_on_limit = true);
