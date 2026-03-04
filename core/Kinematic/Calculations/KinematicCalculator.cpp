#include "KinematicCalculator.h"
#include <cmath>
#include <algorithm>
#include <thread>
#include <atomic>

static double NormDeg(double deg, double cycle)
{
    double x = std::fmod(deg, cycle);
    if (x < 0.0) x += cycle;
    return x;
}

KinematicCalculator::KinematicCalculator(const EngineParams& params)
    : m_params(params)
{
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

std::vector<double> KinematicCalculator::generateAlphaGrid() const
{
    std::vector<double> alpha;
    const int numPoints = static_cast<int>(m_params.end_alpha / m_params.step_alpha) + 1;
    alpha.reserve(numPoints);

    for (int i = 0; i < numPoints; ++i)
        alpha.push_back(i * m_params.step_alpha);

    return alpha;
}

std::vector<double> KinematicCalculator::calculateFiringAngles() const
{
    // КИНЕМАТИКА: используем только геометрию КВ (фазы шеек), а не "вспышки".
    const double cycle = (m_params.taktnost == 2) ? 360.0 : 720.0;

    // Приоритет: новый массив геометрических фаз
    if (m_params.cyl_geom_phase_deg.size() == static_cast<size_t>(m_params.countCyl))
    {
        std::vector<double> angles = m_params.cyl_geom_phase_deg;
        for (double& a : angles) a = NormDeg(a, cycle);
        return angles;
    }

    // Fallback: legacy массив (старые проекты) — трактуем как геометрию
    if (m_params.cyl_phase_deg.size() == static_cast<size_t>(m_params.countCyl))
    {
        std::vector<double> angles = m_params.cyl_phase_deg;
        for (double& a : angles) a = NormDeg(a, cycle);
        return angles;
    }

    // Последний fallback: равномерно (чтобы не падать)
    std::vector<double> angles(m_params.countCyl, 0.0);
    const double interval = cycle / static_cast<double>(m_params.countCyl);
    for (int i = 0; i < m_params.countCyl; ++i)
        angles[i] = interval * static_cast<double>(i);

    return angles;
}

void KinematicCalculator::calculateCylinder(
    int cylinderIndex,
    const std::vector<double>& alpha,
    CalculationResults& results,
    const std::vector<double>& phaseShifts,
    std::atomic<bool>* cancelFlag
) const
{
    if (cancelFlag && cancelFlag->load()) return;

    const double phaseShift = phaseShifts[static_cast<size_t>(cylinderIndex)];

    CylinderResults cylResults;
    m_model->calculate(cylResults, alpha, phaseShift);

    // Переносим результаты
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
}

CalculationResults KinematicCalculator::calculateAll(
    std::function<void(double)> progressCallback,
    std::atomic<bool>* cancelFlag
) const
{
    CalculationResults results;
    results.alpha = generateAlphaGrid();

    const int numCylinders = static_cast<int>(m_params.countCyl);

    // Инициализация векторов
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

    // Геометрические фазы цилиндров (один раз, не внутри каждого цилиндра)
    const std::vector<double> phaseShifts = calculateFiringAngles();

    // Расчёт
    if (m_params.cyl_per_crankpin == 2)
    {
        const int crankCount = numCylinders / 2;

        for (int c = 0; c < crankCount; ++c)
        {
            if (cancelFlag && cancelFlag->load()) break;

            const int base = 2 * c;   // основной цилиндр пары
            const int sideIdx = base + 1;

            calculateCylinder(base, results.alpha, results, phaseShifts, cancelFlag);

            if (m_model->hasSideCylinder())
            {
                // side-результаты лежат в *_side[base] — переносим их в *_full[sideIdx]
                results.cylinder_stroke_full[sideIdx]       = std::move(results.cylinder_stroke_full_side[base]);
                results.cylinder_stroke1[sideIdx]           = std::move(results.cylinder_stroke1_side[base]);
                results.cylinder_stroke2[sideIdx]           = std::move(results.cylinder_stroke2_side[base]);

                results.cylinder_velocity_full[sideIdx]     = std::move(results.cylinder_velocity_full_side[base]);
                results.cylinder_velocity1[sideIdx]         = std::move(results.cylinder_velocity1_side[base]);
                results.cylinder_velocity2[sideIdx]         = std::move(results.cylinder_velocity2_side[base]);

                results.cylinder_acceleration_full[sideIdx] = std::move(results.cylinder_acceleration_full_side[base]);
                results.cylinder_acceleration1[sideIdx]     = std::move(results.cylinder_acceleration1_side[base]);
                results.cylinder_acceleration2[sideIdx]     = std::move(results.cylinder_acceleration2_side[base]);

                results.cylinder_betta_rod[sideIdx]         = std::move(results.cylinder_betta_rod_side[base]);
                results.cylinder_omega_rod[sideIdx]         = std::move(results.cylinder_omega_rod_side[base]);
                results.cylinder_eps_rod[sideIdx]           = std::move(results.cylinder_eps_rod_side[base]);
            }
            else
            {
                // fallback: если модель не отдала side — считаем второй цилиндр как копию первого
                results.cylinder_stroke_full[sideIdx]       = results.cylinder_stroke_full[base];
                results.cylinder_stroke1[sideIdx]           = results.cylinder_stroke1[base];
                results.cylinder_stroke2[sideIdx]           = results.cylinder_stroke2[base];

                results.cylinder_velocity_full[sideIdx]     = results.cylinder_velocity_full[base];
                results.cylinder_velocity1[sideIdx]         = results.cylinder_velocity1[base];
                results.cylinder_velocity2[sideIdx]         = results.cylinder_velocity2[base];

                results.cylinder_acceleration_full[sideIdx] = results.cylinder_acceleration_full[base];
                results.cylinder_acceleration1[sideIdx]     = results.cylinder_acceleration1[base];
                results.cylinder_acceleration2[sideIdx]     = results.cylinder_acceleration2[base];

                results.cylinder_betta_rod[sideIdx]         = results.cylinder_betta_rod[base];
                results.cylinder_omega_rod[sideIdx]         = results.cylinder_omega_rod[base];
                results.cylinder_eps_rod[sideIdx]           = results.cylinder_eps_rod[base];
            }

            if (progressCallback)
                progressCallback(static_cast<double>(c + 1) / std::max(1, crankCount));
        }
    }
    else
    {
        for (int i = 0; i < numCylinders; ++i)
        {
            if (cancelFlag && cancelFlag->load()) break;

            calculateCylinder(i, results.alpha, results, phaseShifts, cancelFlag);

            if (progressCallback)
                progressCallback(static_cast<double>(i + 1) / std::max(1, numCylinders));
        }
    }

    results.firing_interval = (m_params.taktnost == 2)
        ? 360.0 / std::max(1, numCylinders)
        : 720.0 / std::max(1, numCylinders);

    return results;
}