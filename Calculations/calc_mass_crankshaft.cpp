#include "calc_mass_crankshaft.h"
#include <iostream>
#include <cmath>
using namespace std;

void calc_mass_crankshaft(const Params& p) {
    double rho_material = 7850; // кг/м³, сталь

    // Масса коренной шейки
    double vol_root = M_PI * pow(p.diam_root_neck / 2.0, 2) * p.length_root_neck;
    double m_root = rho_material * vol_root;
    double x_root = 0.0;

    // Масса шатунной шейки
    double vol_rod = M_PI * pow(p.diam_rod_neck / 2.0, 2) * p.length_rod_neck;
    double m_rod_neck = rho_material * vol_rod;
    double x_rod = p.dist_axes;

    // Масса щеки с вырезами
    double vol_web_full = p.depth_web * p.width_web * p.dist_axes;
    double vol_fillet = M_PI * pow(p.fillet_rad, 2) * p.depth_web;
    double vol_web_net = vol_web_full - 2 * vol_fillet;
    double m_web_one = rho_material * vol_web_net;
    double m_web_total = m_web_one * 2;
    double x_web = p.dist_axes / 2.0;

    // Общая масса
    double total_mass = m_root + m_rod_neck + m_web_total;

    // Центр тяжести
    double x_cg = (m_root * x_root + m_rod_neck * x_rod + m_web_total * x_web) / total_mass;

    cout << "\n Масса колена вала: " << total_mass << " кг." << endl;
    cout << "\n Расстояние от оси коренной шейки до ЦТ: " << x_cg << " м." << endl;
}
