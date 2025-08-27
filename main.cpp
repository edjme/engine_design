#include <iostream>
#include <cmath>
#include <string>

#include "input_data/input.h"

#include "Calculations/ind_diagr.h"
#include "Calculations/diag_palpha.h"
#include "Calculations/forces_ksm.h"
#include "Calculations/vds_crankpin.h"

#include "Calculations/calc_mass_crankshaft.h"
#include "Calculations/cw_counterweights.h"

int main() {
    #ifdef _WIN32
    // чтобы русские сообщения были читабельны в терминале
    system("chcp 65001 > nul");
    #endif

    // 0) ввод параметров
    Params p = input();

    // 1) индикаторная P–V
    auto ind = build_indicator_PV(
        p,
        /*samples_per_segment*/ 400,
        "output/ind_pv.csv",
        "output/ind_pv.html",
        true
    );
    std::cout << ind.summary << "\n";

    // 2) развёртка P(α)
    auto pal = build_P_alpha(
        p, ind,
        /*step_deg*/ 0.5,
        "output/P_alpha.csv",
        "output/P_alpha.html",
        true
    );
    std::cout << pal.summary << "\n";

    // 3) силы КШМ
    auto fr = build_forces_ksm(
        p, pal,
        "output/forces_ksm.csv",
        "output/forces_ksm.html",
        true
    );
    std::cout << fr.summary << "\n";

    // 4) ВДС шатунной шейки (X=Z+P'c, Y=T)
    auto vds = build_vds_crankpin(
        p, fr,
        "output/vds_crankpin.csv",
        "output/vds_crankpin.html",
        true
    );
    std::cout << vds.summary << "\n";

    // 5) масса/ЦТ колена
    auto cm = calc_mass_crankshaft(p);
    std::cout
        << "\n=== Контрольные расстояния (из модуля масс) ===\n"
        << "Полноопорный:  axis_p=" << (p.length_root_neck + 2.0*p.depth_web + p.length_rod_neck)
        << ", web_p=" << (p.length_root_neck + 3.0*p.depth_web + 2.0*p.length_rod_neck) << "\n"
        << "Неполноопорный:axis_n=" << (p.length_rod_neck + p.depth_web)
        << ", web_n=" << (2.0*p.depth_web + 2.0*p.length_rod_neck) << "\n";

    // базовый STL колена (без прот.) — если нужен
    if (export_crank_STL_mm(p, "output/crank_mm.stl", 128)) {
        std::cout << "STL (base) saved: output/crank_mm.stl\n";
    }

    // 6) противовесы
    CWVariant var = CWVariant::FullSupport_V1;
const long vsel = (long)std::llround(p.config_prot); // берём из CSV
if      (vsel == 2) var = CWVariant::FullSupport_V2;
else if (vsel == 3) var = CWVariant::SemiSupport;

// 3) строим и сохраняем общий STL (миллиметры)
auto cw = build_counterweights_and_export(
    p, cm, var,
    "output/crank_with_cw_mm.stl",
    128,
    /*clamp_alpha_on_limit=*/true
);

if (!cw.ok){
    std::cerr << "[CW] Предупреждение/ошибка: " << cw.message << "\n";
}

    std::cout << "\nГотово.\n";
    return 0;
}
