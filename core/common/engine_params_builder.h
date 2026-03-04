#pragma once
#include "core/common/common_types.h"
#include <string>
#include <vector>

// Чистый ввод для сборки параметров двигателя (без wx / gui)
struct EngineBuildInput
{
    int taktnost = 4;                 // 2 или 4
    int crank_count = 4;              // число кривошипов
    int cyl_per_crankpin = 1;         // 1 или 2
    bool articulated_rod = false;     // прицепной шатун (если cyl_per_crankpin==2)

    std::vector<double> crank_phase_deg; // size=crank_count, 0..cycle
    
std::vector<double> cyl_cycle_phase_deg;  // фаза цикла, град (0..360 для 2Т, 0..720 для 4Т)

    double gamma_deg = 0.0;           
    double dezaxial_m = 0.0;          // дезаксиал

    // параметры прицепного шатуна
    double gamma_pric_deg = 0.0;
    double radcrank1_m = 0.0;
    double lengthRod1_m = 0.0;
    bool full_support_bearings = true;
};

// Собрать EngineParams. Возвращает false если ввод неконсистентен.
bool BuildEngineParamsFromCrank(const EngineBuildInput& in, EngineParams& out, std::string& err);