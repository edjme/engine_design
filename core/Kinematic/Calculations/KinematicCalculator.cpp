#include "KinematicCalculator.h"
#include <cmath>
#include <algorithm>
#include <thread>
#include <atomic>

KinematicCalculator::KinematicCalculator(const EngineParams& params)
    : m_params(params) {
    // Определяем тип КШМ по параметрам
    KSMType type;
    const double e = params.dezaxial;
    const double gamma = params.gamma;
    const double gammaPric = params.gammaPric;
    
    if (e == 0 && gamma == 0 && gammaPric == 0) {
        type = KSMType::Axial;
    } else if (e != 0 && gamma == 0 && gammaPric == 0) {
        type = KSMType::Deaxial;
    } else if (e == 0 && gamma != 0 && gammaPric == 0) {
        type = KSMType::VShaped;
    } else if (e != 0 && gamma != 0 && gammaPric == 0) {
        type = KSMType::VShapedDeaxial;
    } else if (e == 0 && gamma != 0 && gammaPric != 0) {
        type = KSMType::VShapedAttached;
    } else if (e != 0 && gamma != 0 && gammaPric != 0) {
        type = KSMType::VShapedAttachedDeaxial;
    } else {
        type = KSMType::Axial; // По умолчанию
    }
    
    m_model = KSMModel::create(type, params);
}

std::vector<double> KinematicCalculator::generateAlphaGrid() const {
    std::vector<double> alpha;
    int numPoints = static_cast<int>(m_params.end_alpha / m_params.step_alpha) + 1;
    alpha.reserve(numPoints);
    
    for (int i = 0; i < numPoints; ++i) {
        alpha.push_back(i * m_params.step_alpha);
    }
    
    return alpha;
}

std::vector<double> KinematicCalculator::calculateFiringAngles() const {
    std::vector<double> angles(m_params.countCyl, 0.0);
    
    // Интервал между вспышками
    double fullCycleDegrees = (m_params.taktnost == 2) ? 360.0 : 720.0;
    double interval = fullCycleDegrees / m_params.countCyl;
    
    // Равномерное распределение (пока без учёта порядка работы)
    for (int i = 0; i < m_params.countCyl; ++i) {
        angles[i] = i * interval;
    }
    
    return angles;
}

void KinematicCalculator::setProgressCallback(ProgressCallback callback)
{
    m_callback = callback;
}

void KinematicCalculator::calculateCylinder(
    int cylinderIndex,
    const std::vector<double>& alpha,
    CalculationResults& results) const {
    
    if (m_cancelled) return;
    
    std::vector<double> firingAngles = calculateFiringAngles();
    double phaseShift = firingAngles[cylinderIndex];
    
    // Создаём временные результаты для этого цилиндра
    CylinderResults cylResults;
    
    // Выполняем расчёт через модель
    m_model->calculate(cylResults, alpha, phaseShift);
    
    // Переносим результаты в общую структуру
    results.cylinder_stroke_full[cylinderIndex] = cylResults.stroke_full;
    results.cylinder_stroke1[cylinderIndex] = cylResults.stroke1;
    results.cylinder_stroke2[cylinderIndex] = cylResults.stroke2;
    results.cylinder_velocity_full[cylinderIndex] = cylResults.velocity_full;
    results.cylinder_velocity1[cylinderIndex] = cylResults.velocity1;
    results.cylinder_velocity2[cylinderIndex] = cylResults.velocity2;
    results.cylinder_acceleration_full[cylinderIndex] = cylResults.acceleration_full;
    results.cylinder_acceleration1[cylinderIndex] = cylResults.acceleration1;
    results.cylinder_acceleration2[cylinderIndex] = cylResults.acceleration2;
    results.cylinder_betta_rod[cylinderIndex] = cylResults.betta_rod;
    results.cylinder_omega_rod[cylinderIndex] = cylResults.omega_rod;
    results.cylinder_eps_rod[cylinderIndex] = cylResults.eps_rod;
    
    // Если есть боковой цилиндр
    if (m_model->hasSideCylinder() && cylResults.side) {
        results.cylinder_stroke_full_side[cylinderIndex] = cylResults.side->stroke_full;
    results.cylinder_stroke1_side[cylinderIndex] = cylResults.side->stroke1;
    results.cylinder_stroke2_side[cylinderIndex] = cylResults.side->stroke2;
    results.cylinder_velocity_full_side[cylinderIndex] = cylResults.side->velocity_full;
    results.cylinder_velocity1_side[cylinderIndex] = cylResults.side->velocity1;
    results.cylinder_velocity2_side[cylinderIndex] = cylResults.side->velocity2;
    results.cylinder_acceleration_full_side[cylinderIndex] = cylResults.side->acceleration_full;
    results.cylinder_acceleration1_side[cylinderIndex] = cylResults.side->acceleration1;
    results.cylinder_acceleration2_side[cylinderIndex] = cylResults.side->acceleration2;
    results.cylinder_betta_rod_side[cylinderIndex] = cylResults.side->betta_rod;
    results.cylinder_omega_rod_side[cylinderIndex] = cylResults.side->omega_rod;
    results.cylinder_eps_rod_side[cylinderIndex] = cylResults.side->eps_rod;
    }
    
    m_progress = static_cast<double>(cylinderIndex + 1) / m_params.countCyl;
}

void KinematicCalculator::calculateCylinderThread(
    int cylinderIndex,
    const EngineParams& params,
    const std::unique_ptr<KSMModel>& model,
    const std::vector<double>& alpha,
    CalculationResults& results,
    std::atomic<int>& completed,
    std::atomic<bool>& cancelled) {
    
    if (cancelled) return;
    
    std::vector<double> firingAngles; // TODO: передавать как параметр
    double interval = (params.taktnost == 2) ? 360.0 / params.countCyl : 720.0 / params.countCyl;
    double phaseShift = cylinderIndex * interval;
    
    CylinderResults cylResults;
    model->calculate(cylResults, alpha, phaseShift);
    
    // Заполняем результаты (нужна синхронизация!)
    // В реальности нужно использовать мьютексы
    
    completed++;
}

CalculationResults KinematicCalculator::calculateAll() {
    CalculationResults results;
    
    // Генерируем сетку углов
    results.alpha = generateAlphaGrid();
    
    // Инициализируем векторы для каждого цилиндра
    int numCylinders = static_cast<int>(m_params.countCyl);
    results.cylinder_stroke_full.resize(numCylinders);
    results.cylinder_stroke1.resize(numCylinders);
    results.cylinder_stroke2.resize(numCylinders);
    results.cylinder_velocity_full.resize(numCylinders);
    results.cylinder_velocity1.resize(numCylinders);
    results.cylinder_velocity2.resize(numCylinders);
    results.cylinder_acceleration_full.resize(numCylinders);
    results.cylinder_acceleration1.resize(numCylinders);
    results.cylinder_acceleration2.resize(numCylinders);
    results.cylinder_betta_rod.resize(numCylinders);
    results.cylinder_omega_rod.resize(numCylinders);
    results.cylinder_eps_rod.resize(numCylinders);
    
    if (m_model->hasSideCylinder()) {
        results.cylinder_stroke_full_side.resize(numCylinders);
        results.cylinder_stroke1_side.resize(numCylinders);
        results.cylinder_stroke2_side.resize(numCylinders);
        results.cylinder_velocity_full_side.resize(numCylinders);
        results.cylinder_velocity1_side.resize(numCylinders);
        results.cylinder_velocity2_side.resize(numCylinders);
        results.cylinder_acceleration_full_side.resize(numCylinders);
        results.cylinder_acceleration1_side.resize(numCylinders);
        results.cylinder_acceleration2_side.resize(numCylinders);
        results.cylinder_betta_rod_side.resize(numCylinders);
        results.cylinder_omega_rod_side.resize(numCylinders);
        results.cylinder_eps_rod_side.resize(numCylinders);
    }
    
    m_progress = 0.0;
    m_cancelled = false;
    
    if (m_parallel && numCylinders > 1) {
        // Многопоточный расчёт (TODO: реализовать)
        // Пока используем последовательный
        for (int i = 0; i < numCylinders; ++i) {
            calculateCylinder(i, results.alpha, results);
        }
    } else {
        // Последовательный расчёт
        for (int i = 0; i < numCylinders; ++i) {
            calculateCylinder(i, results.alpha, results);
        }
    }
    
    // Заполняем сводные данные для первого цилиндра (для обратной совместимости)
    if (!results.cylinder_stroke_full.empty()) {
        results.stroke_full = results.cylinder_stroke_full[0];
        results.stroke1 = results.cylinder_stroke1[0];
        results.stroke2 = results.cylinder_stroke2[0];
        results.velocity_full = results.cylinder_velocity_full[0];
        results.velocity1 = results.cylinder_velocity1[0];
        results.velocity2 = results.cylinder_velocity2[0];
        results.acceleration_full = results.cylinder_acceleration_full[0];
        results.acceleration1 = results.cylinder_acceleration1[0];
        results.acceleration2 = results.cylinder_acceleration2[0];
        results.betta_rod = results.cylinder_betta_rod[0];
        results.omega_rod = results.cylinder_omega_rod[0];
        results.eps_rod = results.cylinder_eps_rod[0];
    }
    
    if (m_model->hasSideCylinder() && !results.cylinder_stroke_full_side.empty()) {
    results.stroke_full_side = results.cylinder_stroke_full_side[0];
    results.stroke1_side = results.cylinder_stroke1_side[0];
    results.stroke2_side = results.cylinder_stroke2_side[0];
    results.velocity_full_side = results.cylinder_velocity_full_side[0];
    results.velocity1_side = results.cylinder_velocity1_side[0];
    results.velocity2_side = results.cylinder_velocity2_side[0];
    results.acceleration_full_side = results.cylinder_acceleration_full_side[0];
    results.acceleration1_side = results.cylinder_acceleration1_side[0];
    results.acceleration2_side = results.cylinder_acceleration2_side[0];
    results.betta_rod_side = results.cylinder_betta_rod_side[0];
    results.omega_rod_side = results.cylinder_omega_rod_side[0];
    results.eps_rod_side = results.cylinder_eps_rod_side[0];
    }
    
    results.firing_interval = (m_params.taktnost == 2) ? 360.0 / m_params.countCyl : 720.0 / m_params.countCyl;
    
    return results;
}