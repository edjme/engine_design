#ifndef KINEMATIC_FORMULAS_H
#define KINEMATIC_FORMULAS_H

#include "common/common_types.h"

// Главная функция для расчёта кинематики КШМ
// Принимает параметры двигателя и возвращает все результаты расчёта
CalculationResults calcCylinderKinematics(const EngineParams &params);

#endif // KINEMATIC_FORMULAS_H
