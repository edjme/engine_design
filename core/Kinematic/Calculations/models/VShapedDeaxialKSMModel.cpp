#include "VShapedDeaxialKSMModel.h"
#include <cmath>
#include <algorithm>

VShapedDeaxialKSMModel::VShapedDeaxialKSMModel(const EngineParams& params)
    : KSMModel(KSMType::VShapedDeaxial, params) {}

void VShapedDeaxialKSMModel::calculate(CylinderResults& results,
                                       const std::vector<double>& alpha,
                                       double phaseShift) const {
    
    results.clear();
    results.ensureSide();
    
    const double r = m_params.radcrank;
    const double k = m_params.lyambda;
    const double L = (k != 0.0) ? r / k : 1e12;
    const double w = 2 * PI * m_params.n / 60.0;
    const double e = m_params.dezaxial;
    const double z = (r != 0.0) ? e / r : 0.0;
    const double g = deg2rad(m_params.gamma);  // угол развала
    
    size_t n = alpha.size();
    
    // Резервируем память для основного цилиндра
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
    
    // Резервируем память для бокового цилиндра
    results.side->stroke_full.reserve(n);
    results.side->stroke1.reserve(n);
    results.side->stroke2.reserve(n);
    results.side->velocity_full.reserve(n);
    results.side->velocity1.reserve(n);
    results.side->velocity2.reserve(n);
    results.side->acceleration_full.reserve(n);
    results.side->acceleration1.reserve(n);
    results.side->acceleration2.reserve(n);
    results.side->betta_rod.reserve(n);
    results.side->omega_rod.reserve(n);
    results.side->eps_rod.reserve(n);
    
    for (double a_deg : alpha) {
        double a_shifted_deg = fmod(a_deg + 2*phaseShift, 360.0);
        double a = a_shifted_deg * DEG_TO_RAD;
        
        // ===== ГЛАВНЫЙ ЦИЛИНДР (дезаксиальный) =====
        {
            double s_a = std::sin(a);
            double c_a = std::cos(a);
            
            double qM = r * s_a - e;
            double SM = std::sqrt(std::max(0.0, L * L - qM * qM));
            
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
            double vf = w * (r * s_a + (r * c_a) * qM / std::max(1e-15, SM));
            
            results.velocity1.push_back(v1);
            results.velocity2.push_back(v2);
            results.velocity_full.push_back(vf);
            
            // Ускорения
            double a1 = r * w * w * (c_a + k * z * s_a);
            double a2 = r * w * w * k * std::cos(2 * a);
            double af = w * w * (r * c_a - (r * qM * s_a) / std::max(1e-15, SM) 
                        + (r * r * L * L * c_a * c_a) / std::max(1e-45, SM * SM * SM));
            
            results.acceleration1.push_back(a1);
            results.acceleration2.push_back(a2);
            results.acceleration_full.push_back(af);
            
            // Угловые параметры главного шатуна
            double betta = std::asin(k * s_a - k * z);
            results.betta_rod.push_back(betta * RAD_TO_DEG);
            
            double v_rod = k * w * (c_a / std::sqrt(1 - k * k * std::pow(s_a - z, 2)));
            results.omega_rod.push_back(v_rod);
            
            double a_rod = k * w * w * (
                (-s_a * (1 - k * k * std::pow(s_a - z, 2)) + 
                 k * k * c_a * c_a * (s_a - z)) / 
                std::pow(1 - k * k * std::pow(s_a - z, 2), 1.5)
            );
            results.eps_rod.push_back(a_rod);
        }
        
        // ===== БОКОВОЙ ЦИЛИНДР (a - γ) =====
        {
            double ap = a - g;
            double s = std::sin(ap);
            double c = std::cos(ap);
            
            double qS = r * s - e;
            double SS = std::sqrt(std::max(0.0, L * L - qS * qS));
            
            // Перемещения
            double s1 = r * ((1 - c) - k * z * s);
            double s2 = r * 0.25 * k * (1 - std::cos(2 * ap));
            double sf = r * (1 - c) + 
                        std::sqrt(std::max(0.0, L * L - e * e)) - 
                        std::sqrt(std::max(0.0, L * L - (r * s - e) * (r * s - e)));
            
            results.side->stroke1.push_back(s1);
            results.side->stroke2.push_back(s2);
            results.side->stroke_full.push_back(sf);
            
            // Скорости
            double v1 = w * r * (s - k * z * c);
            double v2 = w * r * 0.5 * k * std::sin(2 * ap);
            double vf = w * (r * s + (r * c) * qS / std::max(1e-15, SS));
            
            results.side->velocity1.push_back(v1);
            results.side->velocity2.push_back(v2);
            results.side->velocity_full.push_back(vf);
            
            // Ускорения
            double a1 = r * w * w * (c + k * z * s);
            double a2 = r * w * w * k * std::cos(2 * ap);
            double af = w * w * (r * c - (r * qS * s) / std::max(1e-15, SS) 
                        + (r * r * L * L * c * c) / std::max(1e-45, SS * SS * SS));
            
            results.side->acceleration1.push_back(a1);
            results.side->acceleration2.push_back(a2);
            results.side->acceleration_full.push_back(af);
            
            // Угловые параметры бокового шатуна
            double betta = std::asin(k * s - k * z);
            results.side->betta_rod.push_back(betta * RAD_TO_DEG);
            
            double v_rod = k * w * (c / std::sqrt(1 - k * k * std::pow(s - z, 2)));
            results.side->omega_rod.push_back(v_rod);
            
            double a_rod = k * w * w * (
                (-s * (1 - k * k * std::pow(s - z, 2)) + 
                 k * k * c * c * (s - z)) / 
                std::pow(1 - k * k * std::pow(s - z, 2), 1.5)
            );
            results.side->eps_rod.push_back(a_rod);
        }
    }
}