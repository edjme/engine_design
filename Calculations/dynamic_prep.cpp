#include "dynamic_prep.h"
#include "utils/utils.h"

FullSupportSetup makeFullSupportSetup(const Params& p, const CrankshaftMassResults& cm) {
    FullSupportSetup fs{};

    fs.omega  = p.w;
    fs.R      = p.r;
    fs.lambda = p.lyambda;     // у тебя lyambda = 1/3 — оставляем как задано
    fs.a      = p.a;

    // плоскости цилиндров симметрично относительно центра
    fs.z1 = -fs.a / 2.0;
    fs.z2 =  fs.a / 2.0;

    // фазы: первый цилиндр = 0, второй — из CSV (обычно 180°)
    fs.phi1 = 0.0;
    fs.phi2 = deg2rad(p.phase_cyl2_deg);

    // массы "на цилиндр" в КГ (переводим из кг/м^2 по площади поршня)
    DynMasses m{};
    m.M_rec = arealToMass(p.m_pd,  p.diam_cyl);
    m.M2    = arealToMass(p.m_2,   p.diam_cyl);
    m.M_rot = arealToMass(cm.m_rotating, p.diam_cyl);
    fs.masses = m;

    return fs;
}
