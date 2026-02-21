#include "DeaxialKSMModel.h"
#include <cmath>
#include <algorithm>

DeaxialKSMModel::DeaxialKSMModel(const EngineParams& params)
    : KSMModel(KSMType::Deaxial, params) {}

void DeaxialKSMModel::calculate(CylinderResults& results,
                                const std::vector<double>& alpha,
                                double phaseShift) const {
    
    results.clear();
    
    const double r = m_params.radcrank;
    const double k = m_params.lyambda;
    const double L = (k != 0.0) ? r / k : 1e12;
    const double w = 2 * PI * m_params.n / 60.0;
    const double e = m_params.dezaxial;
    const double z = (r != 0.0) ? e / r : 0.0;
    
    size_t n = alpha.size();
    // ... резервирование памяти как в AxialKSMModel ...
    
    for (double a_deg : alpha) {
        double a_shifted_deg = fmod(a_deg + phaseShift, 360.0);
        double a = a_shifted_deg * DEG_TO_RAD;
        double s_a = std::sin(a);
        double c_a = std::cos(a);
        
        double q = r * s_a - e;
        double S = std::sqrt(std::max(0.0, L * L - q * q));
        
        // Перемещения
        double s1 = r * ((1 - c_a) - k * z * s_a);
        double s2 = r * 0.25 * k * (1 - std::cos(2 * a));
        double sf = r * (1 - c_a) + 
                    std::sqrt(std::max(0.0, L * L - e * e)) - 
                    std::sqrt(std::max(0.0, L * L - (r * s_a - e) * (r * s_a - e)));
        
        results.stroke1.push_back(s1);
        results.stroke2.push_back(s2);
        results.stroke_full.push_back(sf);
        
        // Скорости
        double v1 = w * r * (s_a - k * z * c_a);
        double v2 = w * r * 0.5 * k * std::sin(2 * a);
        double vf = w * (r * s_a + (r * c_a) * q / std::max(1e-15, S));
        
        results.velocity1.push_back(v1);
        results.velocity2.push_back(v2);
        results.velocity_full.push_back(vf);
        
        // Ускорения
        double a1 = r * w * w * (c_a + k * z * s_a);
        double a2 = r * w * w * k * std::cos(2 * a);
        double af = w * w * (r * c_a - (r * q * s_a) / std::max(1e-15, S) 
                    + (r * r * L * L * c_a * c_a) / std::max(1e-45, S * S * S));
        
        results.acceleration1.push_back(a1);
        results.acceleration2.push_back(a2);
        results.acceleration_full.push_back(af);
        
        // Угловые параметры шатуна
        double betta = std::asin(k * s_a - k * z);
        double betta_deg = betta * RAD_TO_DEG;
        results.betta_rod.push_back(betta_deg);
        
        double v_rod = k * w * (c_a / std::sqrt(1 - k * k * std::pow(s_a - z, 2)));
        results.omega_rod.push_back(v_rod);
        
        double a_rod = k * w * w * (
            (-s_a * (1 - k * k * std::pow(s_a - z, 2)) + 
             k * k * c_a * c_a * (s_a - z)) / 
            std::pow(1 - k * k * std::pow(s_a - z, 2), 1.5)
        );
        results.eps_rod.push_back(a_rod);
    }
}