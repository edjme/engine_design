#pragma once
#include <string>
#include "input_data/input.h"
#include "Calculations/ind_diagr.h"
#include "Calculations/diag_palpha.h"
#include "Calculations/forces_ksm.h"
#include "Calculations/vds_crankpin.h"
#include "Calculations/calc_mass_crankshaft.h"
#include "Calculations/cw_counterweights.h"

// Все артефакты кладём в output/
struct RunArtifacts
{
    std::string pv_csv = "output/ind_pv.csv";
    std::string pv_html = "output/ind_pv.html";
    std::string pa_csv = "output/P_alpha.csv";
    std::string pa_html = "output/P_alpha.html";
    std::string f_csv = "output/forces_ksm.csv";
    std::string f_html = "output/forces_ksm.html";
    std::string vds_csv = "output/vds_crankpin.csv";
    std::string vds_html = "output/vds_crankpin.html";
    std::string crank_stl = "output/crank_mm.stl";
    std::string crank_cw_stl = "output/crank_with_cw_mm.stl";
    std::string balance_csv = "output/balance_inertia.csv";
    std::string balance_html = "output/balance_inertia.html"; // зарезервировано
};

struct PipelineResults
{
    IndicatorResults ind;
    PAlphaResults pal;
    ForcesResults fr;
    VDSCrankpinResults vds;
    CrankshaftMassResults cm;
    CWResult cw;
    RunArtifacts files;
};

// Запуск всего пайплайна расчёта с текущими Params.
// auto_open_html=true откроет html-графики после расчёта.
PipelineResults run_full_pipeline(const Params &p, bool auto_open_html);

// Простой TUI (консольный) интерфейс программы.
void run_cli();
