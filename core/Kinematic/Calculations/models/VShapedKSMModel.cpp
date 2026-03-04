#include "VShapedKSMModel.h"
#include <cmath>
#include <algorithm>

VShapedKSMModel::VShapedKSMModel(const EngineParams& params)
    : KSMModel(KSMType::VShaped, params) {}

void VShapedKSMModel::calculate(CylinderResults& results,
                                const std::vector<double>& alpha,
                                double phaseShift) const {
    
    results.clear();
    results.ensureSide();
    
    const double r = m_params.radcrank;
    const double k = m_params.lyambda;
    const double L = (k != 0.0) ? r / k : 1e12;
    const double w = 2 * PI * m_params.n / 60.0;
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
        double a_shifted_deg = normalizeAngleDeg(a_deg + phaseShift);
        double a = a_shifted_deg * DEG_TO_RAD;
        
        // ===== ГЛАВНЫЙ ЦИЛИНДР (как аксиальный) =====
        {
            double s_a = std::sin(a);
            double c_a = std::cos(a);
            double D = std::sqrt(std::max(0.0, 1.0 - (k * s_a) * (k * s_a)));
            
            double s1 = r * (1 - c_a);
            double s2 = r * 0.25 * k * (1 - std::cos(2 * a));
            double sf = r + L - std::sqrt(L * L - (r * s_a) * (r * s_a)) - r * c_a;
            
            double v1 = w * r * s_a;
            double v2 = w * r * 0.5 * k * std::sin(2 * a);
            double vf = w * r * (s_a + (k * s_a * c_a) / std::max(1e-15, D));
            
            double a1 = r * w * w * c_a;
            double a2 = r * w * w * k * std::cos(2 * a);
            double af = w * w * r * (c_a + (k * std::cos(2 * a)) / std::max(1e-15, D) 
                       + (k * k * k) * (s_a * s_a * c_a * c_a) / std::max(1e-45, D * D * D));
            
            results.stroke1.push_back(s1);
            results.stroke2.push_back(s2);
            results.stroke_full.push_back(sf);
            results.velocity1.push_back(v1);
            results.velocity2.push_back(v2);
            results.velocity_full.push_back(vf);
            results.acceleration1.push_back(a1);
            results.acceleration2.push_back(a2);
            results.acceleration_full.push_back(af);
            
            // Угловые параметры главного шатуна
            double betta = std::asin(k * s_a);
            results.betta_rod.push_back(betta * RAD_TO_DEG);
            results.omega_rod.push_back(k * w * (c_a / std::sqrt(1 - k * k * s_a * s_a)));
            results.eps_rod.push_back(-k * w * w * s_a * ((1 - k * k) / 
                                      std::pow(1 - k * k * s_a * s_a, 1.5)));
        }
        
        // ===== БОКОВОЙ ЦИЛИНДР (угол a - γ) =====
        {
            double ap = a - g;
            double s = std::sin(ap);
            double c = std::cos(ap);
            double D = std::sqrt(std::max(0.0, 1.0 - (k * s) * (k * s)));
            
            double s1 = r * (1 - c);
            double s2 = r * 0.25 * k * (1 - std::cos(2 * ap));
            double sf = r + L - std::sqrt(L * L - (r * s) * (r * s)) - r * c;
            
            double v1 = w * r * s;
            double v2 = w * r * 0.5 * k * std::sin(2 * ap);
            double vf = w * r * (s + (k * s * c) / std::max(1e-15, D));
            
            double a1 = r * w * w * c;
            double a2 = r * w * w * k * std::cos(2 * ap);
            double af = w * w * r * (c + (k * std::cos(2 * ap)) / std::max(1e-15, D) 
                       + (k * k * k) * (s * s * c * c) / std::max(1e-45, D * D * D));
            
            // Создаём боковой цилиндр, если его ещё нет
    
            double betta = std::asin(k * s);
    // Используем ->
    results.side->stroke1.push_back(s1);
    results.side->stroke2.push_back(s2);
    results.side->stroke_full.push_back(sf);
    results.side->velocity1.push_back(v1);
    results.side->velocity2.push_back(v2);
    results.side->velocity_full.push_back(vf);
    results.side->acceleration1.push_back(a1);
    results.side->acceleration2.push_back(a2);
    results.side->acceleration_full.push_back(af);
    results.side->betta_rod.push_back(betta * RAD_TO_DEG);
    results.side->omega_rod.push_back(k * w * (c / std::sqrt(1 - k * k * s * s)));
    results.side->eps_rod.push_back(-k * w * w * s * ((1 - k * k) / 
                                           std::pow(1 - k * k * s * s, 1.5)));
        }
    }
}