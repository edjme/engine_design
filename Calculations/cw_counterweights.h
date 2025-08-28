#pragma once
#include <string>
#include "input_data/input.h"
#include "Calculations/calc_mass_crankshaft.h"

enum class CWVariant
{
    FullSupport_V1 = 1,
    FullSupport_V2 = 2,
    SemiSupport = 3
};

struct CWResult
{
    bool ok;
    double alpha_deg;
    double S_prot;
    std::string message;
};

CWResult build_counterweights_and_export(
    const Params &p,
    const CrankshaftMassResults &cm,
    CWVariant var,
    const std::string &stl_path,
    int seg = 128,
    bool clamp_alpha_on_limit = true);
