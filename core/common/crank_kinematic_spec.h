#pragma once
#include <vector>

enum class RodPairType { SideBySide, Articulated };

struct CrankSpec
{
    int index = 0;           // 0..N-1
    double phase_deg = 0.0;  // фаза кривошипа относительно первого, градусы
};

struct EngineKinematicSpec
{
    int taktnost = 4;                 // 2/4
    int cyl_per_crankpin = 1;         // 1/2
    RodPairType rod_pair = RodPairType::SideBySide;

    // геометрия “общая”
    double gamma_deg = 0.0;           // угол развала (для V/оппозит тоже нужен)
    double dezaxial_m = 0.0;          // общий дезаксиал для всех цилиндров

    // параметры прицепного (используются только для второго цилиндра пары, если rod_pair=Articulated)
    double gamma_pric_deg = 0.0;
    double radcrank1_m = 0.0;
    double lengthRod1_m = 0.0;

    // базовые r, λ, n, шаг
    double radcrank_m = 0.0;
    double lambda = 0.0;
    double n_rpm = 0.0;
    double step_alpha_deg = 1.0;

    std::vector<CrankSpec> cranks;    // N кривошипов
};