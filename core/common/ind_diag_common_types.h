#ifndef IND_DIAG_COMMON_TYPES_H
#define IND_DIAG_COMMON_TYPES_H

#include <vector>
#include <iostream>

// Структура для параметров расчета индикаторной диаграммы
struct IndParams
{
    double step_alpha;
    double end_alpha;
    double radcrank;
    double lyambda;
    double diam_cyl; // диаметр цилиндра, м
    double n;
    double epsilent; // степень сжатия ε
    // Давления (абсолютные), Па
    double p_a; // на впуске
    double p_r; // на выпуске
    // Показатели политропы
    double n_1; // сжатие
    double n_2; // расширение

    // Коэффициенты индикаторной диаграммы
    double lymbda_z; // степень повышения давления (Pz/Pc)
    double ro;       // степень предварительного расширения (Vz'/Vz)

    double tau; // тактность (2 или 4)

    // Конструктор по умолчанию
    IndParams();
};

// Результаты построения индикаторной диаграммы P–V.
// Единицы: V — м^3, P — Па, работы/теплоты — Дж.
struct IndicatorResults
{
    // Базовые величины
    double F_p; // площадь поршня, м^2
    double Vh;  // рабочий объём, м^3
    double Vc;  // объём камеры, м^3
    double Va;  // объём в нижней мёртвой точке, м^3

    // Давления опорных точек (Па)
    double Pa;  // впуск
    double Pr;  // выпуск
    double Pc;  // конец сжатия
    double Pz;  // пик/уровень изобары
    double Pz_; // изобарный участок (обычно = Pz)
    double Pb;  // конец расширения

    // Объёмы ключевых точек (м^3)
    double Vz;  // объём в ВМТ
    double Vz_; // объём после изобарного подогрева

    // Участки для визуализации (гладкие куски)
    std::vector<double> V_comp, P_comp;       // a -> c (сжатие)
    std::vector<double> V_iso_add, P_iso_add; // c -> z (изохора)
    std::vector<double> V_preexp, P_preexp;   // z -> z' (изобара)
    std::vector<double> V_exp, P_exp;         // z' -> b (расширение)
    std::vector<double> V_drop_b, P_drop_b;   // b -> r' (блоудаун)
    std::vector<double> V_exh, P_exh;         // r' -> r (выпуск, 4Т)
    std::vector<double> V_drop_r, P_drop_r;   // r -> r'' (вертикаль к Pa)
    std::vector<double> V_int, P_int;         // r'' -> a (впуск)

    // Итоговый путь (соединение всех участков по выбранной тактности)
    std::vector<double> V_path;
    std::vector<double> P_path;

    // Интегралы (на один цилиндр, за цикл)
    double A_cycle; // индикаторная работа, Дж
    double Q_in;    // подвод теплоты (по принятой схеме), Дж
    double eta;     // КПД индикаторный (A_cycle/Q_in)

    // Диагностика
    std::string summary;
};

#endif