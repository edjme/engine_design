#include "AxialKSMModel.h"
#include <cmath>
#include <algorithm>

AxialKSMModel::AxialKSMModel(const EngineParams& params)
    : KSMModel(KSMType::Axial, params) {}

void AxialKSMModel::calculate(CylinderResults& results,
                              const std::vector<double>& alpha,
                              double phaseShift) const {
    
    // Очищаем результаты
    results.clear();
    
    const double r = m_params.radcrank;
    const double k = m_params.lyambda;            // r/L
    const double L = (k != 0.0) ? r / k : 1e12;
    const double w = 2 * PI * m_params.n / 60.0;  // угловая скорость
    
    // Резервируем память для оптимизации
    size_t n = alpha.size();
    results.stroke_full.reserve(n);
    results.stroke1.reserve(n);
    results.stroke2.reserve(n);
    results.velocity_full.reserve(n);
    results.velocity1.reserve(n);
    results.velocity2.reserve(n);
    results.acceleration_full.reserve(n);
    results.acceleration1.reserve(n);
    results.acceleration2.reserve(n);
    results.betta_rod.reserve(n);
    results.omega_rod.reserve(n);
    results.eps_rod.reserve(n);
    
    for (double a_deg : alpha) {
        // Смещаем угол на фазу цилиндра
        double a_shifted_deg = normalizeAngleDeg(a_deg + phaseShift);
        double a = a_shifted_deg * DEG_TO_RAD;
        double s_a = std::sin(a);
        double c_a = std::cos(a);
        
        double D = std::sqrt(std::max(0.0, 1.0 - (k * s_a) * (k * s_a)));
        
        // Перемещения
        double s1 = r * (1 - c_a);
        double s2 = r * 0.25 * k * (1 - std::cos(2 * a));
        double sf = r + L - std::sqrt(L * L - (r * s_a) * (r * s_a)) - r * c_a;
        
        results.stroke1.push_back(s1);
        results.stroke2.push_back(s2);
        results.stroke_full.push_back(sf);
        
        // Скорости
        double v1 = w * r * s_a;
        double v2 = w * r * 0.5 * k * std::sin(2 * a);
        double vf = w * r * (s_a + (k * s_a * c_a) / std::max(1e-15, D));
        
        results.velocity1.push_back(v1);
        results.velocity2.push_back(v2);
        results.velocity_full.push_back(vf);
        
        // Ускорения
        double a1 = r * w * w * c_a;
        double a2 = r * w * w * k * std::cos(2 * a);
        double af = w * w * r * (c_a + (k * std::cos(2 * a)) / std::max(1e-15, D) 
                   + (k * k * k) * (s_a * s_a * c_a * c_a) / std::max(1e-45, D * D * D));
        
        results.acceleration1.push_back(a1);
        results.acceleration2.push_back(a2);
        results.acceleration_full.push_back(af);
        
        // Угловые параметры шатуна
        double betta = std::asin(k * s_a);
        double betta_deg = betta * RAD_TO_DEG;
        results.betta_rod.push_back(betta_deg);
        
        double v_rod = k * w * (c_a / std::sqrt(1 - k * k * s_a * s_a));
        results.omega_rod.push_back(v_rod);
        
        double a_rod = -k * w * w * s_a * ((1 - k * k) / 
                       std::pow(1 - k * k * s_a * s_a, 1.5));
        results.eps_rod.push_back(a_rod);
    }
}