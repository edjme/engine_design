#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <vector>
#include <iostream>

// Структура для параметров расчета для кинематики


struct EngineParams
{
    double step_alpha = 10.0;
    double end_alpha = 360.0;
    double radcrank = 0.020;
    double lyambda = 0.3;
    double n = 4800.0;
    double gamma = 0.0;
    double gammaPric = 0.0;
    double dezaxial = 0.0;
    double radcrank1 = 0.0;
    double lengthRod1 = 0.0;
    int countCyl = 2;
    int taktnost = 4;

    // --- фазы цилиндров (сдвиги по углу коленвала) ---
    // В системе программы: alpha=0 соответствует ВМТ конца сжатия цилиндра 1.
    // Размер должен быть == countCyl. Если пусто — используем равномерное распределение.
    // --- фазы ---
// Геометрическая фаза: сдвиг по углу коленвала (геометрия шеек), цикл 360°.
// Цикловая фаза: сдвиг рабочего цикла (для динамики/давления), цикл 360° (2Т) или 720° (4Т).
int layout_rows = 1;
int layout_sections = 1;

// legacy (оставляем, чтобы не ломать старый код/GUI): используем как ГЕОМЕТРИЮ
std::vector<double> cyl_phase_deg;        // legacy, град (0..360/720), сейчас = cyl_geom_phase_deg

std::vector<double> cyl_geom_phase_deg;   // геометрия КВ, град (0..360)

std::vector<double> cyl_cycle_phase_deg;  // фаза цикла, град (0..360 для 2Т, 0..720 для 4Т)
bool has_cycle_phase = false;             // когда позже введёшь углы вспышки — выставляй true
double tdc_tolerance_deg = 1.0;


    // --- конфигурация коленвала/цилиндров (новая модель ввода) ---
int  cyl_per_crankpin = 1;        // 1 или 2
bool articulated_rod  = false;    // прицепной шатун
bool full_support_bearings = true; // на будущее

};

// Структура для результатов расчетов кинематики
struct CalculationResults
{
    // Главный цилиндр
    [[deprecated("Use cylinder_stroke_full[0] instead")]] std::vector<double> stroke_full;
    [[deprecated("Use cylinder_stroke1[0] instead")]] std::vector<double> stroke1;
    [[deprecated("Use cylinder_stroke2[0] instead")]] std::vector<double> stroke2;
    [[deprecated("Use cylinder_velocity_full[0] instead")]] std::vector<double> velocity_full;
    [[deprecated("Use cylinder_velocity1[0] instead")]] std::vector<double> velocity1;
    [[deprecated("Use cylinder_velocity2[0] instead")]] std::vector<double> velocity2;
    [[deprecated("Use cylinder_acceleration_full[0] instead")]] std::vector<double> acceleration_full;
    [[deprecated("Use cylinder_acceleration1[0] instead")]] std::vector<double> acceleration1;
    [[deprecated("Use cylinder_acceleration1[0] instead")]] std::vector<double> acceleration2;

    // Боковой цилиндр (только при gamma != 0)
    [[deprecated("Use cylinder_stroke_full_side[0] instead")]] std::vector<double> stroke_full_side;
    [[deprecated("Use cylinder_stroke1_side[0] instead")]] std::vector<double> stroke1_side;
    [[deprecated("Use cylinder_stroke2_side[0] instead")]] std::vector<double> stroke2_side;
    [[deprecated("Use cylinder_velocity_full_side[0] instead")]] std::vector<double> velocity_full_side;
    [[deprecated("Use cylinder_velocity1_side[0] instead")]] std::vector<double> velocity1_side;
    [[deprecated("Use cylinder_velocity2_side[0] instead")]] std::vector<double> velocity2_side;
    [[deprecated("Use cylinder_acceleration_full_side[0] instead")]] std::vector<double> acceleration_full_side;
    [[deprecated("Use cylinder_acceleration1_side[0] instead")]] std::vector<double> acceleration1_side;
    [[deprecated("Use cylinder_acceleration2_side[0] instead")]] std::vector<double> acceleration2_side;

    // Угловые перещения, скорость, ускорение шатуна
    [[deprecated("Use cylinder_betta_rod[0] instead")]] std::vector<double> betta_rod;
    [[deprecated("Use cylinder_omega_rod[0] instead")]] std::vector<double> omega_rod;
    [[deprecated("Use cylinder_eps_rod[0] instead")]] std::vector<double> eps_rod;

    // Для бокового цилиндра
    [[deprecated("Use cylinder_betta_rod_side[0] instead")]] std::vector<double> betta_rod_side;
    [[deprecated("Use cylinder_omega_rod_side[0] instead")]] std::vector<double> omega_rod_side;
    [[deprecated("Use cylinder_eps_rod_side[0] instead")]] std::vector<double> eps_rod_side;

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