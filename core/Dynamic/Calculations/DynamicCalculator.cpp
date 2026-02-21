#include "DynamicCalculator.h"
#include <cmath>
#include <algorithm>

#define M_PI 3.14159265358979323846

DynamicCalculator::DynamicCalculator(const EngineParams& engineParams,
                                     const CalculationResults& kinematicResults,
                                     const std::vector<double>& pressureAngles,
                                     const std::vector<double>& pressureValues,
                                     double massPiston,
                                     double massRod,
                                     double kRodOsc,
                                    double bore)
    : m_engineParams(engineParams)
    , m_kinematicResults(kinematicResults)
    , m_pressureAngles(pressureAngles)
    , m_pressureValues(pressureValues)
    , m_massPiston(massPiston)
    , m_massRod(massRod)
    , m_kRodOsc(kRodOsc)
{
    m_pistonArea = M_PI * bore * bore / 4.0;;
}

double DynamicCalculator::interpolatePressure(double alpha) const
{
    if (m_pressureAngles.empty()) return 101325.0; // атмосферное давление

    double alphaMod = alpha;
    // Если двигатель 4-тактный, а данные только на 360°, циклически повторяем
    if (m_engineParams.taktnost == 4 && m_pressureAngles.back() <= 360.0) {
        alphaMod = fmod(alpha, 360.0);
    }
    // Если угол выходит за пределы, берём крайнее значение
    if (alphaMod <= m_pressureAngles.front()) return m_pressureValues.front();
    if (alphaMod >= m_pressureAngles.back()) return m_pressureValues.back();

    // Бинарный поиск
    auto it = std::upper_bound(m_pressureAngles.begin(), m_pressureAngles.end(), alphaMod);
    size_t idx = it - m_pressureAngles.begin();
    double a1 = m_pressureAngles[idx-1], a2 = m_pressureAngles[idx];
    double p1 = m_pressureValues[idx-1], p2 = m_pressureValues[idx];
    return p1 + (p2 - p1) * (alphaMod - a1) / (a2 - a1);
}

DynamicResults DynamicCalculator::calculate()
{
    DynamicResults results;
    results.clear();

    size_t numCyl = m_engineParams.countCyl;
    size_t numPoints = m_kinematicResults.alpha.size();

    // Инициализация векторов
    results.gas_force.resize(numCyl);
    results.inertia_force.resize(numCyl);
    results.total_force.resize(numCyl);
    results.rod_force.resize(numCyl);
    results.tangential_force.resize(numCyl);
    results.radial_force.resize(numCyl);
    results.cylinder_torque.resize(numCyl);
    results.total_torque.resize(numPoints, 0.0);

    // Вычисляем возвратно-поступательную массу
    double m_recip = m_massPiston + m_kRodOsc * m_massRod;

    // Фазовые углы цилиндров (как в кинематике)
    double fullCycle = (m_engineParams.taktnost == 2) ? 360.0 : 720.0;
    double firingInterval = fullCycle / numCyl;
    std::vector<double> phaseShifts(numCyl);
    for (size_t i = 0; i < numCyl; ++i) {
        phaseShifts[i] = i * firingInterval;
    }

    for (size_t i = 0; i < numPoints; ++i)
    {
        if (m_cancelled) break;

        double alpha = m_kinematicResults.alpha[i];
        double torqueSum = 0.0;

        for (size_t cyl = 0; cyl < numCyl; ++cyl)
        {
            // Угол цилиндра с учётом фазы
            double alphaCyl = alpha + phaseShifts[cyl];

            // Давление в цилиндре
            double pressure = interpolatePressure(alphaCyl);

            // Сила давления газов (принимаем, что с другой стороны поршня атмосферное давление)
            double forceGas = (pressure - 101325.0) * m_pistonArea;

            // Сила инерции
            double accel = m_kinematicResults.cylinder_acceleration_full[cyl][i];
            double forceInertia = -m_recip * accel;

            // Суммарная сила
            double forceTotal = forceGas + forceInertia;

            // Угол отклонения шатуна
            double beta = m_kinematicResults.cylinder_betta_rod[cyl][i] * M_PI / 180.0; // в радианы

            // Сила вдоль шатуна
            double forceRod = forceTotal / cos(beta);

            // Тангенциальная и радиальная силы
            double alphaRad = alphaCyl * M_PI / 180.0;
            double forceTan = forceRod * sin(alphaRad + beta);
            double forceRad = forceRod * cos(alphaRad + beta);

            // Момент от цилиндра
            double torque = forceTan * m_engineParams.radcrank;

            // Сохраняем
            results.gas_force[cyl].push_back(forceGas);
            results.inertia_force[cyl].push_back(forceInertia);
            results.total_force[cyl].push_back(forceTotal);
            results.rod_force[cyl].push_back(forceRod);
            results.tangential_force[cyl].push_back(forceTan);
            results.radial_force[cyl].push_back(forceRad);
            results.cylinder_torque[cyl].push_back(torque);

            // Суммируем момент для этого угла
            torqueSum += torque;
        }

        results.total_torque[i] = torqueSum;

        // Прогресс
        if (m_callback)
        {
            double progress = (i + 1.0) / numPoints;
            bool cancel = false;
            m_callback(progress, cancel);
            if (cancel) m_cancelled = true;
        }
    }

    // Заполняем данные для первого цилиндра (для совместимости)
    if (!results.gas_force.empty()) {
        results.gas_force_cyl0 = results.gas_force[0];
        results.inertia_force_cyl0 = results.inertia_force[0];
        results.total_force_cyl0 = results.total_force[0];
        results.rod_force_cyl0 = results.rod_force[0];
        results.tangential_force_cyl0 = results.tangential_force[0];
        results.radial_force_cyl0 = results.radial_force[0];
        results.cylinder_torque_cyl0 = results.cylinder_torque[0];
    }

    results.piston_area = m_pistonArea;
    results.mass_reciprocating = m_recip;

    return results;
}