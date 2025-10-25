#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <vector>

// Структура для параметров расчета
struct EngineParams
{
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

    // Конструктор по умолчанию
    EngineParams();
};

// Структура для результатов расчетов
struct CalculationResults
{
    // Главный цилиндр
    std::vector<double> alpha;
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
};

#endif