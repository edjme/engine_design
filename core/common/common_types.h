#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <vector>
#include <iostream>

// Структура для параметров расчета для кинематики
struct EngineParams
{
    EngineParams() = default;
    double step_alpha;
    double end_alpha;
    double radcrank;
    double lyambda;
    double n;
    double gamma;
    double gammaPric;
    double dezaxial;
    double radcrank1;
    double lengthRod1;
    double countCyl;
    double taktnost;

};

// Структура для результатов расчетов кинематики
struct CalculationResults
{
    // Главный цилиндр
    std::vector<double> stroke_full;
    std::vector<double> stroke1;
    std::vector<double> stroke2;
    std::vector<double> velocity_full;
    std::vector<double> velocity1;
    std::vector<double> velocity2;
    std::vector<double> acceleration_full;
    std::vector<double> acceleration1;
    std::vector<double> acceleration2;

    // Боковой цилиндр (только при gamma != 0)
    std::vector<double> stroke_full_side;
    std::vector<double> stroke1_side;
    std::vector<double> stroke2_side;
    std::vector<double> velocity_full_side;
    std::vector<double> velocity1_side;
    std::vector<double> velocity2_side;
    std::vector<double> acceleration_full_side;
    std::vector<double> acceleration1_side;
    std::vector<double> acceleration2_side;

    // Угловые перещения, скорость, ускорение шатуна
    std::vector<double> betta_rod;
    std::vector<double> omega_rod;
    std::vector<double> eps_rod;

    // Для бокового цилиндра
    std::vector<double> betta_rod_side;
    std::vector<double> omega_rod_side;
    std::vector<double> eps_rod_side;

    std::vector<double> alpha;
    std::vector<double> firing_angles; // Углы чередования для каждого цилиндра
    double firing_interval; // Угловой интервал между вспышками

    // Результаты для каждого цилиндра
    std::vector<std::vector<double>> cylinder_stroke_full;
    std::vector<std::vector<double>> cylinder_stroke1;
    std::vector<std::vector<double>> cylinder_stroke2;
    std::vector<std::vector<double>> cylinder_velocity_full;
    std::vector<std::vector<double>> cylinder_velocity1;
    std::vector<std::vector<double>> cylinder_velocity2;
    std::vector<std::vector<double>> cylinder_acceleration_full;
    std::vector<std::vector<double>> cylinder_acceleration1;
    std::vector<std::vector<double>> cylinder_acceleration2;
    
    // Угловые перемещения, скорость, ускорение шатуна для каждого цилиндра
    std::vector<std::vector<double>> cylinder_betta_rod;
    std::vector<std::vector<double>> cylinder_omega_rod;
    std::vector<std::vector<double>> cylinder_eps_rod;
    
    // Боковые цилиндры (если есть)
    std::vector<std::vector<double>> cylinder_stroke_full_side;
    std::vector<std::vector<double>> cylinder_stroke1_side;
    std::vector<std::vector<double>> cylinder_stroke2_side;
    std::vector<std::vector<double>> cylinder_velocity_full_side;
    std::vector<std::vector<double>> cylinder_velocity1_side;
    std::vector<std::vector<double>> cylinder_velocity2_side;
    std::vector<std::vector<double>> cylinder_acceleration_full_side;
    std::vector<std::vector<double>> cylinder_acceleration1_side;
    std::vector<std::vector<double>> cylinder_acceleration2_side;
    std::vector<std::vector<double>> cylinder_betta_rod_side;
    std::vector<std::vector<double>> cylinder_omega_rod_side;
    std::vector<std::vector<double>> cylinder_eps_rod_side;
};

struct DynamicResults {
    // Для каждого цилиндра (векторы векторов)
    std::vector<std::vector<double>> gas_force;        // сила давления газов, Н
    std::vector<std::vector<double>> inertia_force;    // сила инерции, Н
    std::vector<std::vector<double>> total_force;      // суммарная сила, Н
    std::vector<std::vector<double>> rod_force;        // сила вдоль шатуна, Н
    std::vector<std::vector<double>> tangential_force; // тангенциальная сила, Н
    std::vector<std::vector<double>> radial_force;     // радиальная сила, Н
    std::vector<std::vector<double>> cylinder_torque;  // крутящий момент от одного цилиндра, Н·м

    // Суммарный момент двигателя (один вектор по углам)
    std::vector<double> total_torque;                   // Н·м

    // Для первого цилиндра (для обратной совместимости при отображении)
    std::vector<double> gas_force_cyl0;
    std::vector<double> inertia_force_cyl0;
    std::vector<double> total_force_cyl0;
    std::vector<double> rod_force_cyl0;
    std::vector<double> tangential_force_cyl0;
    std::vector<double> radial_force_cyl0;
    std::vector<double> cylinder_torque_cyl0;

    // Параметры, использованные при расчёте
    double piston_area;        // площадь поршня, м²
    double mass_reciprocating; // возвратно-поступательная масса, кг

    void clear() {
        gas_force.clear();
        inertia_force.clear();
        total_force.clear();
        rod_force.clear();
        tangential_force.clear();
        radial_force.clear();
        cylinder_torque.clear();
        total_torque.clear();
        gas_force_cyl0.clear();
        inertia_force_cyl0.clear();
        total_force_cyl0.clear();
        rod_force_cyl0.clear();
        tangential_force_cyl0.clear();
        radial_force_cyl0.clear();
        cylinder_torque_cyl0.clear();
    }
};

#endif