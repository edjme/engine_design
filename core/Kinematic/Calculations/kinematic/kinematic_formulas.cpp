// kinematic_formulas.cpp
#include "kinematic_formulas.h"
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std;

// ===== ВНУТРЕННИЕ ПРОТОТИПЫ =====
static void calcAxialKSM(const EngineParams &params, CalculationResults &results);
static void calcDeaxialKSM(const EngineParams &params, CalculationResults &results);
static void calcVTypeKSM(const EngineParams &params, CalculationResults &results);
static void calcVTypeDeaxialKSM(const EngineParams &params, CalculationResults &results);
static void calcVTypeAttachedKSM(const EngineParams &params, CalculationResults &results);
static void calcVTypeAttachedDeaxialKSM(const EngineParams &params, CalculationResults &results);

// ===== ОСНОВНАЯ ФУНКЦИЯ =====
CalculationResults calcCylinderKinematics(const EngineParams &params)
{
    CalculationResults results;

    // Угловая сетка
    for (double a = 0; a <= params.end_alpha + 1e-9; a += params.step_alpha)
        results.alpha.push_back(a);

    const double e = params.dezaxial;
    const double gamma = params.gamma;
    const double gammaPric = params.gammaPric;

    if (e == 0 && gamma == 0 && gammaPric == 0)
        calcAxialKSM(params, results);
    else if (e != 0 && gamma == 0 && gammaPric == 0)
        calcDeaxialKSM(params, results);
    else if (e == 0 && gamma != 0 && gammaPric == 0)
        calcVTypeKSM(params, results);
    else if (e != 0 && gamma != 0 && gammaPric == 0)
        calcVTypeDeaxialKSM(params, results);
    else if (e == 0 && gamma != 0 && gammaPric != 0)
        calcVTypeAttachedKSM(params, results);
    else if (e != 0 && gamma != 0 && gammaPric != 0)
        calcVTypeAttachedDeaxialKSM(params, results);

    return results;
}

// ===== ВСПОМОГАТЕЛЬНЫЕ МЕЛОЧИ =====
const double M_PI = 3.14159265358979323846;
static inline double deg2rad(double deg) { return deg * M_PI / 180.0; }

// =====================================================================
// 1) АКСИАЛЬНЫЙ КШМ (e = 0, γ = 0, γp = 0)
// Полные формулы
// =====================================================================
static void calcAxialKSM(const EngineParams &params, CalculationResults &results)
{
    const double pi = M_PI;
    const double DEG_TO_RAD = pi / 180.0;
    const double RAD_TO_DEG = 180.0 / pi;

    const double r = params.radcrank;
    const double k = params.lyambda;            // r/L
    const double L = (k != 0.0) ? r / k : 1e12; // перестраховка от деления на 0
    const double w = 2 * pi * params.n / 60.0;  // угловая скорость

    for (double a_deg : results.alpha)
    {
        double a = a_deg * DEG_TO_RAD;
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

        // Ускорения (точная формула)
        double a1 = r * w * w * c_a;
        double a2 = r * w * w * k * std::cos(2 * a);
        // Полное через вторую производную строгого выражения смещения:
        // a_full = w^2 * r * ( cos(a) + k*cos(2a)/D + k^3 * (sin^2(a) * cos^2(a))/D^3 )
        // (эквивалент строго дифференцированному виду)
        double af = w * w * r * (c_a + (k * std::cos(2 * a)) / std::max(1e-15, D) + (k * k * k) * (s_a * s_a * c_a * c_a) / std::max(1e-45, D * D * D));

        results.acceleration1.push_back(a1);
        results.acceleration2.push_back(a2);
        results.acceleration_full.push_back(af);

        //  УГЛОВЫЕ ПЕРЕМЕЩЕНИЯ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА
        // Угловое перемещение шатуна
        double betta = asin(k * s_a);
        double betta_deg = betta * RAD_TO_DEG;
        results.betta_rod.push_back(betta_deg);

        // Угловая скорость шатуна
        double v_rod = k * w * ((cos(a)) / sqrt(1 - k * k * s_a * s_a));
        results.omega_rod.push_back(v_rod);

        // Угловое ускорение шатуна
        double a_rod = -k * pow(w, 2) * s_a * ((1 - k * k) / (pow((1 - k * k * s_a * s_a), 3 / 2)));
        results.eps_rod.push_back(a_rod);
    }
}

// =====================================================================
// 2) ДЕЗАКСИАЛЬНЫЙ КШМ (e ≠ 0, γ = 0, γp = 0)
// Полные формулы с q, S и т.п.
// =====================================================================
static void calcDeaxialKSM(const EngineParams &params, CalculationResults &results)
{
    const double pi = M_PI;
    const double DEG_TO_RAD = pi / 180.0;
    const double RAD_TO_DEG = 180.0 / pi;

    const double r = params.radcrank;
    const double k = params.lyambda; // r/L
    const double L = (k != 0.0) ? r / k : 1e12;
    const double w = 2 * pi * params.n / 60.0;
    const double e = params.dezaxial;
    const double z = (r != 0.0) ? e / r : 0.0;

    for (double a_deg : results.alpha)
    {
        double a = a_deg * DEG_TO_RAD;
        double s_a = std::sin(a);
        double c_a = std::cos(a);

        double q = r * s_a - e;
        double S = std::sqrt(std::max(0.0, L * L - q * q));

        // Перемещения
        double s1 = r * ((1 - c_a) - k * z * s_a);
        double s2 = r * 0.25 * k * (1 - std::cos(2 * a));
        double sf = r * (1 - c_a) + std::sqrt(std::max(0.0, L * L - e * e)) - std::sqrt(std::max(0.0, L * L - (r * s_a - e) * (r * s_a - e)));

        results.stroke1.push_back(s1);
        results.stroke2.push_back(s2);
        results.stroke_full.push_back(sf);

        // Скорости
        double v1 = w * r * (s_a - k * z * c_a);
        double v2 = w * r * 0.5 * k * std::sin(2 * a);
        double vf = w * (r * s_a + (r * c_a) * (q) / std::max(1e-15, S));

        results.velocity1.push_back(v1);
        results.velocity2.push_back(v2);
        results.velocity_full.push_back(vf);

        // Ускорения (строгое дифференцирование)
        double a1 = r * w * w * (c_a + k * z * s_a);
        double a2 = r * w * w * k * std::cos(2 * a);
        // a_full = w^2 * [ r*cos(a) - (r*q*sin(a))/S + (r^2 * L^2 * cos^2(a)) / S^3 ]
        double af = w * w * (r * c_a - (r * q * s_a) / std::max(1e-15, S) + (r * r * L * L * c_a * c_a) / std::max(1e-45, S * S * S));

        results.acceleration1.push_back(a1);
        results.acceleration2.push_back(a2);
        results.acceleration_full.push_back(af);

        //  УГЛОВЫЕ ПЕРЕМЕЩЕНИЯ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА
        // Угловое перемещение шатуна
        double betta = asin(k * s_a - k * z);
        double betta_deg = betta * RAD_TO_DEG;
        results.betta_rod.push_back(betta_deg);

        // Угловая скорость шатуна
        double v_rod = k * w * ((cos(a)) / sqrt(1 - k * k * pow(s_a - z, 2)));
        results.omega_rod.push_back(v_rod);

        // Угловое ускорение шатуна
        double a_rod = k * pow(w, 2) * ((-s_a * (1 - pow(k, 2) * pow(s_a - z, 2)) + pow(k, 2) * pow(c_a, 2) * (s_a - z)) / pow((1 - k * k * pow(s_a - z, 2)), 3 / 2));
        results.eps_rod.push_back(a_rod);
    }
}

// =====================================================================
// 3) V-ОБРАЗНЫЙ КШМ (рядом сидящие шатуны) (e = 0, γ ≠ 0, γp = 0)
// Полные формулы для главного и бокового
// =====================================================================
static void calcVTypeKSM(const EngineParams &params, CalculationResults &results)
{
    const double pi = M_PI;
    const double DEG_TO_RAD = pi / 180.0;
    const double RAD_TO_DEG = 180.0 / pi;

    const double r = params.radcrank;
    const double k = params.lyambda; // r/L
    const double L = (k != 0.0) ? r / k : 1e12;
    const double w = 2 * pi * params.n / 60.0;
    const double g = deg2rad(params.gamma);

    for (double a_deg : results.alpha)
    {
        double a = a_deg * DEG_TO_RAD;

        // ===== Главный цилиндр (как аксиальный) =====
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
            double af = w * w * r * (c_a + (k * std::cos(2 * a)) / std::max(1e-15, D) + (k * k * k) * (s_a * s_a * c_a * c_a) / std::max(1e-45, D * D * D));

            results.stroke1.push_back(s1);
            results.stroke2.push_back(s2);
            results.stroke_full.push_back(sf);

            results.velocity1.push_back(v1);
            results.velocity2.push_back(v2);
            results.velocity_full.push_back(vf);

            results.acceleration1.push_back(a1);
            results.acceleration2.push_back(a2);
            results.acceleration_full.push_back(af);

            //  УГЛОВЫЕ ПЕРЕМЕЩЕНИЯ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА
            // Угловое перемещение шатуна
            double betta = asin(k * s_a);
            double betta_deg = betta * RAD_TO_DEG;
            results.betta_rod.push_back(betta_deg);

            // Угловая скорость шатуна
            double v_rod = k * w * ((cos(a)) / sqrt(1 - k * k * s_a * s_a));
            results.omega_rod.push_back(v_rod);

            // Угловое ускорение шатуна
            double a_rod = -k * pow(w, 2) * s_a * ((1 - k * k) / (pow((1 - k * k * s_a * s_a), 3 / 2)));
            results.eps_rod.push_back(a_rod);
        }

        // ===== Боковой цилиндр (угол a - γ) =====
        {
            double ap = a - g; // угол a - γ
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
            double af = w * w * r * (c + (k * std::cos(2 * ap)) / std::max(1e-15, D) + (k * k * k) * (s * s * c * c) / std::max(1e-45, D * D * D));

            results.stroke1_side.push_back(s1);
            results.stroke2_side.push_back(s2);
            results.stroke_full_side.push_back(sf);

            results.velocity1_side.push_back(v1);
            results.velocity2_side.push_back(v2);
            results.velocity_full_side.push_back(vf);

            results.acceleration1_side.push_back(a1);
            results.acceleration2_side.push_back(a2);
            results.acceleration_full_side.push_back(af);

            //  УГЛОВЫЕ ПЕРЕМЕЩЕНИЯ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА
            // Угловое перемещение шатуна
            double betta = asin(k * s);
            double betta_deg = betta * RAD_TO_DEG;
            results.betta_rod_side.push_back(betta_deg);

            // Угловая скорость шатуна
            double v_rod = k * w * ((cos(a)) / sqrt(1 - k * k * s * s));
            results.omega_rod_side.push_back(v_rod);

            // Угловое ускорение шатуна
            double a_rod = -k * pow(w, 2) * s * ((1 - k * k) / (pow((1 - k * k * s * s), 3 / 2)));
            results.eps_rod_side.push_back(a_rod);
        }
    }
}

// =====================================================================
// 4) V-ОБРАЗНЫЙ ДЕЗАКСИАЛЬНЫЙ (e ≠ 0, γ ≠ 0, γp = 0)
// Полные формулы для главного и бокового с q,S
// =====================================================================
static void calcVTypeDeaxialKSM(const EngineParams &params, CalculationResults &results)
{
    const double pi = M_PI;
    const double DEG_TO_RAD = pi / 180.0;
    const double RAD_TO_DEG = 180.0 / pi;

    const double r = params.radcrank;
    const double k = params.lyambda; // r/L
    const double L = (k != 0.0) ? r / k : 1e12;
    const double w = 2 * pi * params.n / 60.0;
    const double e = params.dezaxial;
    const double z = (r != 0.0) ? e / r : 0.0;
    const double g = deg2rad(params.gamma);

    for (double a_deg : results.alpha)
    {
        double a = a_deg * DEG_TO_RAD;

        // ===== Главный цилиндр =====
        {
            double s_a = std::sin(a);
            double c_a = std::cos(a);

            double qM = r * s_a - e;
            double SM = std::sqrt(std::max(0.0, L * L - qM * qM));

            double s1 = r * ((1 - c_a) - k * z * s_a);
            double s2 = r * 0.25 * k * (1 - std::cos(2 * a));
            double sf = r * (1 - c_a) + std::sqrt(std::max(0.0, L * L - e * e)) - std::sqrt(std::max(0.0, L * L - (r * s_a - e) * (r * s_a - e)));

            double v1 = w * r * (s_a - k * z * c_a);
            double v2 = w * r * 0.5 * k * std::sin(2 * a);
            double vf = w * (r * s_a + (r * c_a) * (qM) / std::max(1e-15, SM));

            double a1 = r * w * w * (c_a + k * z * s_a);
            double a2 = r * w * w * k * std::cos(2 * a);
            double af = w * w * (r * c_a - (r * qM * s_a) / std::max(1e-15, SM) + (r * r * L * L * c_a * c_a) / std::max(1e-45, SM * SM * SM));

            results.stroke1.push_back(s1);
            results.stroke2.push_back(s2);
            results.stroke_full.push_back(sf);

            results.velocity1.push_back(v1);
            results.velocity2.push_back(v2);
            results.velocity_full.push_back(vf);

            results.acceleration1.push_back(a1);
            results.acceleration2.push_back(a2);
            results.acceleration_full.push_back(af);

            //  УГЛОВЫЕ ПЕРЕМЕЩЕНИЯ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА
            // Угловое перемещение шатуна
            double betta = asin(k * s_a - k * z);
            double betta_deg = betta * RAD_TO_DEG;
            results.betta_rod.push_back(betta_deg);

            // Угловая скорость шатуна
            double v_rod = k * w * ((cos(a)) / sqrt(1 - k * k * pow(s_a - z, 2)));
            results.omega_rod.push_back(v_rod);

            // Угловое ускорение шатуна
            double a_rod = k * pow(w, 2) * ((-s_a * (1 - pow(k, 2) * pow(s_a - z, 2)) + pow(k, 2) * pow(c_a, 2) * (s_a - z)) / pow((1 - k * k * pow(s_a - z, 2)), 3 / 2));
            results.eps_rod.push_back(a_rod);
        }

        // ===== Боковой цилиндр (a - γ) =====
        {
            double ap = a - g; // угол a - γ
            double s = std::sin(ap);
            double c = std::cos(ap);

            double qS = r * s - e;
            double SS = std::sqrt(std::max(0.0, L * L - qS * qS));

            double s1 = r * ((1 - c) - k * z * s);
            double s2 = r * 0.25 * k * (1 - std::cos(2 * ap));
            double sf = r * (1 - c) + std::sqrt(std::max(0.0, L * L - e * e)) - std::sqrt(std::max(0.0, L * L - (r * s - e) * (r * s - e)));

            double v1 = w * r * (s - k * z * c);
            double v2 = w * r * 0.5 * k * std::sin(2 * ap);
            double vf = w * (r * s + (r * c) * (qS) / std::max(1e-15, SS));

            double a1 = r * w * w * (c + k * z * s);
            double a2 = r * w * w * k * std::cos(2 * ap);
            double af = w * w * (r * c - (r * qS * s) / std::max(1e-15, SS) + (r * r * L * L * c * c) / std::max(1e-45, SS * SS * SS));

            results.stroke1_side.push_back(s1);
            results.stroke2_side.push_back(s2);
            results.stroke_full_side.push_back(sf);

            results.velocity1_side.push_back(v1);
            results.velocity2_side.push_back(v2);
            results.velocity_full_side.push_back(vf);

            results.acceleration1_side.push_back(a1);
            results.acceleration2_side.push_back(a2);
            results.acceleration_full_side.push_back(af);

            //  УГЛОВЫЕ ПЕРЕМЕЩЕНИЯ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА
            // Угловое перемещение шатуна
            double betta = asin(k * s - k * z);
            double betta_deg = betta * RAD_TO_DEG;
            results.betta_rod.push_back(betta_deg);

            // Угловая скорость шатуна
            double v_rod = k * w * ((cos(a)) / sqrt(1 - k * k * pow(s - z, 2)));
            results.omega_rod.push_back(v_rod);

            // Угловое ускорение шатуна
            double a_rod = k * pow(w, 2) * ((-s * (1 - pow(k, 2) * pow(s - z, 2)) + pow(k, 2) * pow(c, 2) * (s - z)) / pow((1 - k * k * pow(s - z, 2)), 3 / 2));
            results.eps_rod.push_back(a_rod);
        }
    }
}

// =====================================================================
// 5) V-ОБРАЗНЫЙ С ПРИЦЕПНЫМ ШАТУНОМ (e = 0, γ ≠ 0, γp ≠ 0)
// Полные формулы (строгая кинематика с β, β', β'')
// =====================================================================
static void calcVTypeAttachedKSM(const EngineParams &params, CalculationResults &results)
{
    const double pi = M_PI;
    const double DEG_TO_RAD = pi / 180.0;
    const double RAD_TO_DEG = 180.0 / pi;

    // Главный кривошип/шатун
    const double r = params.radcrank;
    const double k = params.lyambda; // r/L
    const double L = (k != 0.0) ? r / k : 1e12;

    // Прицепной шатун/второй кривошип
    const double r1 = params.radcrank1;
    const double L1 = params.lengthRod1;
    const double lyambda1 = (L1 != 0.0) ? r1 / L1 : 0.0;

    const double w = 2 * pi * params.n / 60.0;

    const double g = deg2rad(params.gamma);
    const double gp = deg2rad(params.gammaPric);
    const double tet = g - gp;

    for (double a_deg : results.alpha)
    {
        double a = a_deg * DEG_TO_RAD;

        // ===== Главный цилиндр (как аксиальный) =====
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
            double af = w * w * r * (c_a + (k * std::cos(2 * a)) / std::max(1e-15, D) + (k * k * k) * (s_a * s_a * c_a * c_a) / std::max(1e-45, D * D * D));

            results.stroke1.push_back(s1);
            results.stroke2.push_back(s2);
            results.stroke_full.push_back(sf);

            results.velocity1.push_back(v1);
            results.velocity2.push_back(v2);
            results.velocity_full.push_back(vf);

            results.acceleration1.push_back(a1);
            results.acceleration2.push_back(a2);
            results.acceleration_full.push_back(af);

            //  УГЛОВЫЕ ПЕРЕМЕЩЕНИЯ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА
            // Угловое перемещение шатуна
            double betta = asin(k * s_a);
            double betta_deg = betta * RAD_TO_DEG;
            results.betta_rod.push_back(betta_deg);

            // Угловая скорость шатуна
            double v_rod = k * w * ((cos(a)) / sqrt(1 - k * k * s_a * s_a));
            results.omega_rod.push_back(v_rod);

            // Угловое ускорение шатуна
            double a_rod = -k * pow(w, 2) * s_a * ((1 - k * k) / (pow((1 - k * k * s_a * s_a), 3 / 2)));
            results.eps_rod.push_back(a_rod);
        }

        // ===== Боковой цилиндр (прицепной) с точными производными =====
        {
            double argS = a - g - gp; // угол a - γ - γ1

            // геометрия двухшатунного сочленения
            double kPric = (L1 != 0.0) ? (r / L1) : 0.0; // отношение главного кривошипа к плечу прицепного шатуна
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
            double betapp = (-(r / L) * std::sin(a)) / std::max(eps, cosB) + ((r / L) * (r / L) * std::cos(a) * std::cos(a) * sinB) / std::max(eps, cosB * cosB * cosB);

            // β1 (для прицепного шатуна) через зависимость sin β1:
            // sinβ1 = kPric*sin(a - g) - delta*( sin(tet)*cosB + cos(tet)*sinB )
            double sinB1 = kPric * std::sin(a - g) - delta * (std::sin(tet) * cosB + std::cos(tet) * sinB);
            double cosB1 = std::sqrt(std::max(0.0, 1.0 - sinB1 * sinB1));

            // β1' и β1''
            double betap1 = (r * std::cos(a - g) - r1 * cosBpD * betap) / std::max(eps, (L1 * cosB1));
            double betapp1 = (-r * std::sin(a - g) + L1 * sinB1 * betap1 * betap1 + r1 * sinBpD * betap * betap - r1 * cosBpD * betapp) / std::max(eps, (L1 * cosB1));

            // dx'/dα (амплитуда смещения прицепного поршня; x' — координата)
            double dxp_dalpha = -r * std::sin(a - g) - r1 * sinBpD * betap - L1 * sinB1 * betap1;
            double d2xp_dalpha2 = -r * std::cos(a - g) - r1 * (cosBpD * betap * betap + sinBpD * betapp) - L1 * (cosB1 * betap1 * betap1 + sinB1 * betapp1);

            // Перемещения
            double s1_side = r1 * (1 - std::cos(argS));
            double s2_side = r1 * 0.25 * lyambda1 * (1 - std::cos(2 * argS));
            // Полное: разложенная геометрия по звеньям (главный + прицепной + шарнир)
            double sf_side = r * (1 - std::cos(a - g)) + r1 * (1 - (std::cos(tet) * cosB - std::sin(tet) * sinB)) + L1 * (1 - cosB1);

            results.stroke1_side.push_back(s1_side);
            results.stroke2_side.push_back(s2_side);
            results.stroke_full_side.push_back(sf_side);

            // Скорости
            double v1_side = w * r1 * std::sin(argS);
            double v2_side = w * r1 * 0.5 * lyambda1 * std::sin(2 * argS);
            double vf_side = -w * dxp_dalpha;

            results.velocity1_side.push_back(v1_side);
            results.velocity2_side.push_back(v2_side);
            results.velocity_full_side.push_back(vf_side);

            // Ускорения
            double a1_side = r1 * w * w * std::cos(argS);
            double a2_side = r1 * w * w * lyambda1 * std::cos(2 * argS);
            double af_side = -w * w * d2xp_dalpha2;

            results.acceleration1_side.push_back(a1_side);
            results.acceleration2_side.push_back(a2_side);
            results.acceleration_full_side.push_back(af_side);

            //  УГЛОВЫЕ ПЕРЕМЕЩЕНИЯ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА
            // Угловое перемещение шатуна
            double betta = asin(kPric * sin(a - g) - delta * k * sin(a));
            double betta_deg = betta * RAD_TO_DEG;
            results.betta_rod_side.push_back(betta_deg);

            // Угловая скорость шатуна
            double v_rod = w * (kPric * cos(a - g) - delta * k * cos(a)) / cosB1;
            results.omega_rod_side.push_back(v_rod);

            // Угловое ускорение шатуна
            double a_rod = (sinB1 / cosB1) * pow(v_rod, 2) + w * w * (-kPric * sin(a - g) + delta * k * sin(a));
            results.eps_rod_side.push_back(a_rod);
        }
    }
}

// =====================================================================
// 6) V-ОБРАЗНЫЙ С ПРИЦЕПНЫМ ШАТУНОМ + ДЕЗАКСИАЛЬНОСТЬ (e ≠ 0, γ ≠ 0, γp ≠ 0)
// Полные формулы (как в п.5 + учёт e в β1)
// =====================================================================
static void calcVTypeAttachedDeaxialKSM(const EngineParams &params, CalculationResults &results)
{
    const double pi = M_PI;
    const double DEG_TO_RAD = pi / 180.0;
    const double RAD_TO_DEG = 180.0 / pi;

    // Главный кривошип/шатун
    const double r = params.radcrank;
    const double k = params.lyambda; // r/L
    const double L = (k != 0.0) ? r / k : 1e12;

    // Прицепной шатун/второй кривошип
    const double r1 = params.radcrank1;
    const double L1 = params.lengthRod1;
    const double lyambda1 = (L1 != 0.0) ? r1 / L1 : 0.0;

    const double w = 2 * pi * params.n / 60.0;

    const double e = params.dezaxial;
    const double z = (r != 0.0) ? e / r : 0.0;

    const double g = deg2rad(params.gamma);
    const double gp = deg2rad(params.gammaPric);
    const double tet = g - gp;

    for (double a_deg : results.alpha)
    {
        double a = a_deg * DEG_TO_RAD;

        // ===== Главный цилиндр (деаксиальный) =====
        {
            double s_a = std::sin(a);
            double c_a = std::cos(a);

            double qM = r * s_a - e;
            double SM = std::sqrt(std::max(0.0, L * L - qM * qM));

            double s1 = r * ((1 - c_a) - k * z * s_a);
            double s2 = r * 0.25 * k * (1 - std::cos(2 * a));
            double sf = r * (1 - c_a) + std::sqrt(std::max(0.0, L * L - e * e)) - std::sqrt(std::max(0.0, L * L - (r * s_a - e) * (r * s_a - e)));

            double v1 = w * r * (s_a - k * z * c_a);
            double v2 = w * r * 0.5 * k * std::sin(2 * a);
            double vf = w * (r * s_a + (r * c_a) * (qM) / std::max(1e-15, SM));

            double a1 = r * w * w * (c_a + k * z * s_a);
            double a2 = r * w * w * k * std::cos(2 * a);
            double af = w * w * (r * c_a - (r * qM * s_a) / std::max(1e-15, SM) + (r * r * L * L * c_a * c_a) / std::max(1e-45, SM * SM * SM));

            results.stroke1.push_back(s1);
            results.stroke2.push_back(s2);
            results.stroke_full.push_back(sf);

            results.velocity1.push_back(v1);
            results.velocity2.push_back(v2);
            results.velocity_full.push_back(vf);

            results.acceleration1.push_back(a1);
            results.acceleration2.push_back(a2);
            results.acceleration_full.push_back(af);

            //  УГЛОВЫЕ ПЕРЕМЕЩЕНИЯ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА
            // Угловое перемещение шатуна
            double betta = asin(k * s_a - k * z);
            double betta_deg = betta * RAD_TO_DEG;
            results.betta_rod.push_back(betta_deg);

            // Угловая скорость шатуна
            double v_rod = k * w * ((cos(a)) / sqrt(1 - k * k * pow(s_a - z, 2)));
            results.omega_rod.push_back(v_rod);

            // Угловое ускорение шатуна
            double a_rod = k * pow(w, 2) * ((-s_a * (1 - pow(k, 2) * pow(s_a - z, 2)) + pow(k, 2) * pow(c_a, 2) * (s_a - z)) / pow((1 - k * k * pow(s_a - z, 2)), 3 / 2));
            results.eps_rod.push_back(a_rod);
        }

        // ===== Боковой цилиндр (прицепной) с учётом e в β1 =====
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
            double betapp = (-(r / L) * std::sin(a)) / std::max(eps, cosB) + ((r / L) * (r / L) * std::cos(a) * std::cos(a) * sinB) / std::max(eps, cosB * cosB * cosB);

            // β1 с учётом e/L1:
            // sinβ1 = kPric*sin(a - g) - delta*( sin(tet)*cosB + cos(tet)*sinB ) - e/L1
            double sinB1 = kPric * std::sin(a - g) - delta * (std::sin(tet) * cosB + std::cos(tet) * sinB) - ((L1 != 0.0) ? (e / L1) : 0.0);
            double cosB1 = std::sqrt(std::max(0.0, 1.0 - sinB1 * sinB1));

            // β1' и β1''
            double betap1 = (r * std::cos(a - g) - r1 * cosBpD * betap) / std::max(eps, (L1 * cosB1));
            double betapp1 = (-r * std::sin(a - g) + L1 * sinB1 * betap1 * betap1 + r1 * sinBpD * betap * betap - r1 * cosBpD * betapp) / std::max(eps, (L1 * cosB1));

            // dx'/dα и d²x'/dα²
            double dxp_dalpha = -r * std::sin(a - g) - r1 * sinBpD * betap - L1 * sinB1 * betap1;
            double d2xp_dalpha2 = -r * std::cos(a - g) - r1 * (cosBpD * betap * betap + sinBpD * betapp) - L1 * (cosB1 * betap1 * betap1 + sinB1 * betapp1);

            // Перемещения (строгая геометрия)
            double s1_side = r1 * ((1 - std::cos(argS)) - lyambda1 * ((r1 != 0.0) ? (e / r1) : 0.0) * std::sin(argS)); // гармоники+прибл. для справки; полный — ниже
            double s2_side = r1 * 0.25 * lyambda1 * (1 - std::cos(2 * argS));

            // Полное (как и в варианте без e, но β1 содержит -e/L1, поэтому смещение учтено)
            double sf_side = r * (1 - std::cos(a - g)) + r1 * (1 - (std::cos(tet) * cosB - std::sin(tet) * sinB)) + L1 * (1 - cosB1);

            results.stroke1_side.push_back(s1_side);
            results.stroke2_side.push_back(s2_side);
            results.stroke_full_side.push_back(sf_side);

            // Скорости
            double v1_side = w * r1 * (std::sin(argS) - lyambda1 * ((r1 != 0.0) ? (e / r1) : 0.0) * std::cos(argS));
            double v2_side = w * r1 * 0.5 * lyambda1 * std::sin(2 * argS);
            double vf_side = -w * dxp_dalpha;

            results.velocity1_side.push_back(v1_side);
            results.velocity2_side.push_back(v2_side);
            results.velocity_full_side.push_back(vf_side);

            // Ускорения
            double a1_side = r1 * w * w * (std::cos(argS) + lyambda1 * ((r1 != 0.0) ? (e / r1) : 0.0) * std::sin(argS));
            double a2_side = r1 * w * w * lyambda1 * std::cos(2 * argS);
            double af_side = -w * w * d2xp_dalpha2;

            results.acceleration1_side.push_back(a1_side);
            results.acceleration2_side.push_back(a2_side);
            results.acceleration_full_side.push_back(af_side);

            //  УГЛОВЫЕ ПЕРЕМЕЩЕНИЯ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА

            double beta1 = std::asin(sinB1);
            double beta1_deg = beta1 * RAD_TO_DEG;

            // Угловая скорость/ускорение прицепного шатуна
            double omega_rod = w * betap1;
            double eps_rod = w * w * betapp1;

            // Сохранение результатов
            results.betta_rod_side.push_back(beta1_deg);
            results.omega_rod_side.push_back(omega_rod);
            results.eps_rod_side.push_back(eps_rod);
        }
    }
}
