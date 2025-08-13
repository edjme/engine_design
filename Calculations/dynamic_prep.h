#pragma once
#include "input_data/input.h"
#include "calc_mass_crankshaft.h"

// Массы на цилиндр в КИЛОГРАММАХ (после перевода из кг/м^2)
struct DynMasses {
    double M_rec;   // поступательная (из m_pd)
    double M2;      // вращающаяся часть шатуна (из m_2)
    double M_rot;   // вращающаяся приведённая масса колена (из cm.m_rotating)
};

// Подготовка данных для полноопорного 2×4Т
struct FullSupportSetup {
    double omega;       // рад/с
    double R;           // м (радиус кривошипа)
    double lambda;      // геом. характеристика КШМ (как в твоих данных p.lyambda)
    double a;           // плечо между плоскостями цилиндров (м)
    double z1, z2;      // координаты плоскостей цилиндров относительно центра (м)
    double phi1, phi2;  // фазы цилиндров (рад)
    DynMasses masses;   // масс-параметры на цилиндр (кг)
};

FullSupportSetup makeFullSupportSetup(const Params& p, const CrankshaftMassResults& cm);
