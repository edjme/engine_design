#pragma once
#include <string>
#include "input_data/input.h"
#include "Calculations/calc_mass_crankshaft.h"   // для CrankshaftMassResults

enum class CWVariant : int {
    FullSupport_V1 = 1,   // два сектора — под обеими щеками
    FullSupport_V2 = 2,   // один сектор — под правой щекой
    SemiSupport    = 3    // неполноопорный: один сектор — под левой щекой, правая щека — половинка
};

struct CWSummary {
    double S_prot{};          // статический момент
    double alpha_rad{};       // угол сектора (рад)
    double alpha_deg{};       // угол сектора (град)
    bool   ok{true};
    std::string message;      // предупреждение/ошибка
};

// Строит ВЕСЬ коленвал (шейки + щеки) и противовесы в одном STL (единицы — миллиметры).
// seg — дискретизация окружностей; если clamp_alpha_on_limit=true, аргумент asin() будет
// аккуратно зажат в [-1, +1] (чтобы не упасть из-за округлений).
CWSummary build_counterweights_and_export(
    const Params& p,
    const CrankshaftMassResults& cm,
    CWVariant var,
    const std::string& stl_path,
    int seg = 128,
    bool clamp_alpha_on_limit = true
);
