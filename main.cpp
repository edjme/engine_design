#include <iostream>
#include <string>
#include "input_data/input.h"
#include "Calculations/ind_diagr.h"        // build_indicator_PV
#include "Calculations/diag_palpha.h"      // build_p_alpha
#include "Calculations/forces_ksm.h"       // build_forces_ksm
#include "Calculations/vds_crankpin.h"     // build_vds_crankpin
#include "Calculations/calc_mass_crankshaft.h" // calc_mass_crankshaft, export_crank_STL_mm

int main(){
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    // 1) входные данные
    Params p = input();

    // удобные пути
    const std::string dir = "output/";
    const std::string pv_csv     = dir + "indicator_curve.csv";
    const std::string pv_html    = dir + "indicator_chart.html";

    const std::string palpha_csv = dir + "p_alpha.csv";
    const std::string palpha_html= dir + "p_alpha.html";

    const std::string forces_csv = dir + "forces.csv";
    const std::string forces_html= dir + "forces.html";

    const std::string vds_csv    = dir + "vds_crankpin.csv";
    const std::string vds_html   = dir + "vds_crankpin.html";

    const std::string crank_stl  = dir + "crank_mm.stl";

    // 2) индикаторная P–V
    auto ind = build_indicator_PV(
        p,
        /*samples_per_segment*/ 400,
        pv_csv, pv_html,
        /*auto_open_html*/ true
    );
    std::cout << ind.summary << "\n";

    // 3) развёртка P(α) (использует опорные точки из индикаторной)
    auto pal = build_P_alpha(
        p, ind,
        /*step_deg*/ 0.5,
        palpha_csv, palpha_html,
        /*auto_open_html*/ true
    );
    std::cout << pal.summary << "\n";

    // 4) силы в КШМ + моменты на коленвалу
    auto fr = build_forces_ksm(
        p, pal,
        forces_csv, forces_html,
        /*auto_open_html*/ true
    );
    std::cout << fr.summary << "\n";

    // 5) ВДС шатунной шейки (X = Z + P′c, Y = T)
    auto vdscp = build_vds_crankpin(
        p, fr,
        vds_csv, vds_html,
        /*auto_open_html*/ true
    );
    std::cout << vdscp.summary << "\n";

    // 6) масса/ЦТ колена вала + STL (в мм)
    auto cm = calc_mass_crankshaft(p);
    std::cout
        << "\n=== Колено вала ===\n"
        << "Масса, кг: " << cm.total_mass << "\n"
        << "Расстояние ЦТ от оси коренной, м: " << cm.x_cg << "\n"
        << "Приведённая масса к коренной, кг/м^2: " << cm.m_root_reduce << "\n"
        << "Масса вращающихся частей, кг/м^2: " << cm.m_rotating << "\n";

    if (export_crank_STL_mm(p, crank_stl, /*seg*/128)) {
        std::cout << "STL сохранён: " << crank_stl << "\n";
    } else {
        std::cout << "Не удалось записать STL: " << crank_stl << "\n";
    }

    // 7) вспомогательные расстояния (для контроля компоновки)
    const double axis_p = p.length_root_neck + 2.0*p.depth_web + p.length_rod_neck;
    const double web_p  = p.length_root_neck + 3.0*p.depth_web + 2.0*p.length_rod_neck;
    const double axis_n = p.length_rod_neck  + p.depth_web;
    const double web_n  = 2.0*p.depth_web    + 2.0*p.length_rod_neck;

    std::cout
        << "\n=== Контрольные расстояния ===\n"
        << "Полноопорный:  axis_p = " << axis_p << " м,  web_p = " << web_p << " м\n"
        << "Неполноопорный:axis_n = " << axis_n << " м,  web_n = " << web_n << " м\n";

    std::cout << "\nГотово.\n";
    return 0;
}
