#include "VShapedAttachedDeaxialKSMModel.h"
#include <cmath>
#include <algorithm>

VShapedAttachedDeaxialKSMModel::VShapedAttachedDeaxialKSMModel(const EngineParams& params)
    : KSMModel(KSMType::VShapedAttachedDeaxial, params) {}

void VShapedAttachedDeaxialKSMModel::calculate(CylinderResults& results,
                                               const std::vector<double>& alpha,
                                               double phaseShift ) const {
    
    results.clear();
    results.ensureSide();
    
    // Главный кривошип/шатун
    const double r = m_params.radcrank;
    const double k = m_params.lyambda; // r/L
    const double L = (k != 0.0) ? r / k : 1e12;
    
    // Прицепной шатун/второй кривошип
    const double r1 = m_params.radcrank1;
    const double L1 = m_params.lengthRod1;
    const double lyambda1 = (L1 != 0.0) ? r1 / L1 : 0.0;
    
    const double w = 2 * PI * m_params.n / 60.0;
    
    const double e = m_params.dezaxial;
    const double z = (r != 0.0) ? e / r : 0.0;
    
    const double g = deg2rad(m_params.gamma);
    const double gp = deg2rad(m_params.gammaPric);
    const double tet = g - gp;
    
    size_t n = alpha.size();
    
    // Резервируем память
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
        double a_shifted_deg = normalizeAngleDeg(a_deg + phaseShift);
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
        
        // ===== БОКОВОЙ ЦИЛИНДР (прицепной с дезаксиалом) =====
        {
            double argS = a - g - gp; // угол a - γ - γ1
            
            // геометрия
            double kPric = (L1 != 0.0) ? (r / L1) : 0.0;
            double delta = (L1 != 0.0) ? (r1 / L1) : 0.0;
            
            // β, β' для главного звена
            double sinB = k * std::sin(a);
            double cosB = std::sqrt(std::max(0.0, 1.0 - sinB * sinB));
            
            // sin(β + θ), cos(β + θ)
            double sD = std::sin(tet), cD = std::cos(tet);
            double sinBpD = sinB * cD + cosB * sD;
            double cosBpD = cosB * cD - sinB * sD;
            
            // β' и β''
            const double eps = 1e-12;
            double betap = ((r / L) * std::cos(a)) / std::max(eps, cosB);
            double betapp = (-(r / L) * std::sin(a)) / std::max(eps, cosB) 
                          + ((r / L) * (r / L) * std::cos(a) * std::cos(a) * sinB) 
                          / std::max(eps, cosB * cosB * cosB);
            
            // β1 с учётом e/L1
            double sinB1 = kPric * std::sin(a - g) 
                         - delta * (std::sin(tet) * cosB + std::cos(tet) * sinB) 
                         - ((L1 != 0.0) ? (e / L1) : 0.0);
            double cosB1 = std::sqrt(std::max(0.0, 1.0 - sinB1 * sinB1));
            
            // β1' и β1''
            double betap1 = (r * std::cos(a - g) - r1 * cosBpD * betap) 
                          / std::max(eps, (L1 * cosB1));
            double betapp1 = (-r * std::sin(a - g) + L1 * sinB1 * betap1 * betap1 
                            + r1 * sinBpD * betap * betap - r1 * cosBpD * betapp) 
                           / std::max(eps, (L1 * cosB1));
            
            // dx'/dα и d²x'/dα²
            double dxp_dalpha = -r * std::sin(a - g) - r1 * sinBpD * betap 
                               - L1 * sinB1 * betap1;
            double d2xp_dalpha2 = -r * std::cos(a - g) 
                                 - r1 * (cosBpD * betap * betap + sinBpD * betapp) 
                                 - L1 * (cosB1 * betap1 * betap1 + sinB1 * betapp1);
            
            // Перемещения (первая гармоника для справки)
            double s1_side = r1 * ((1 - std::cos(argS)) 
                           - lyambda1 * ((r1 != 0.0) ? (e / r1) : 0.0) * std::sin(argS));
            double s2_side = r1 * 0.25 * lyambda1 * (1 - std::cos(2 * argS));
            
            // Полное перемещение (строгая геометрия)
            double sf_side = r * (1 - std::cos(a - g)) 
                           + r1 * (1 - (std::cos(tet) * cosB - std::sin(tet) * sinB)) 
                           + L1 * (1 - cosB1);
            
            results.side->stroke1.push_back(s1_side);
            results.side->stroke2.push_back(s2_side);
            results.side->stroke_full.push_back(sf_side);
            
            // Скорости
            double v1_side = w * r1 * (std::sin(argS) 
                            - lyambda1 * ((r1 != 0.0) ? (e / r1) : 0.0) * std::cos(argS));
            double v2_side = w * r1 * 0.5 * lyambda1 * std::sin(2 * argS);
            double vf_side = -w * dxp_dalpha;
            
            results.side->velocity1.push_back(v1_side);
            results.side->velocity2.push_back(v2_side);
            results.side->velocity_full.push_back(vf_side);
            
            // Ускорения
            double a1_side = r1 * w * w * (std::cos(argS) 
                            + lyambda1 * ((r1 != 0.0) ? (e / r1) : 0.0) * std::sin(argS));
            double a2_side = r1 * w * w * lyambda1 * std::cos(2 * argS);
            double af_side = -w * w * d2xp_dalpha2;
            
            results.side->acceleration1.push_back(a1_side);
            results.side->acceleration2.push_back(a2_side);
            results.side->acceleration_full.push_back(af_side);
            
            // Угловые параметры прицепного шатуна
            double beta1 = std::asin(sinB1);
            results.side->betta_rod.push_back(beta1 * RAD_TO_DEG);
            results.side->omega_rod.push_back(w * betap1);
            results.side->eps_rod.push_back(w * w * betapp1);
        }
    }
}