#include "DynamicCalculator.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double DynamicCalculator::normDeg(double deg, double cycle)
{
    double x = std::fmod(deg, cycle);
    if (x < 0.0) x += cycle;
    return x;
}

double DynamicCalculator::minCircularDistDeg(double a, double b, double cycle)
{
    double d = std::abs(a - b);
    d = std::fmod(d, cycle);
    return std::min(d, cycle - d);
}

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
    m_pistonArea = M_PI * bore * bore / 4.0;
}

double DynamicCalculator::interpolatePressure(double alphaNorm) const
{
    if (m_pressureAngles.empty() || m_pressureValues.empty())
        return 101325.0;

    const size_t n = std::min(m_pressureAngles.size(), m_pressureValues.size());
    if (n == 0)
        return 101325.0;

    // предполагаем, что m_pressureAngles отсортирован по возрастанию
    if (alphaNorm <= m_pressureAngles.front()) return m_pressureValues.front();
    if (alphaNorm >= m_pressureAngles[n - 1]) return m_pressureValues[n - 1];

    auto it = std::upper_bound(m_pressureAngles.begin(), m_pressureAngles.begin() + n, alphaNorm);
    size_t idx = static_cast<size_t>(it - m_pressureAngles.begin());

    if (idx == 0) return m_pressureValues.front();
    if (idx >= n) return m_pressureValues[n - 1];

    const double a1 = m_pressureAngles[idx - 1];
    const double a2 = m_pressureAngles[idx];
    const double p1 = m_pressureValues[idx - 1];
    const double p2 = m_pressureValues[idx];

    const double da = a2 - a1;
    if (std::abs(da) < 1e-12)
        return p1;

    return p1 + (p2 - p1) * (alphaNorm - a1) / da;
}

DynamicResults DynamicCalculator::calculate(std::function<void(double)> progressCallback,
                                            std::atomic<bool>* cancelFlag) const
{
    DynamicResults results;
    results.clear();

    const size_t numCyl = static_cast<size_t>(m_engineParams.countCyl);
    const size_t numPoints = m_kinematicResults.alpha.size();

    // --- allocate ---
    results.gas_force.resize(numCyl);
    results.inertia_force.resize(numCyl);
    results.total_force.resize(numCyl);
    results.rod_force.resize(numCyl);
    results.tangential_force.resize(numCyl);
    results.radial_force.resize(numCyl);
    results.cylinder_torque.resize(numCyl);
    results.total_torque.assign(numPoints, 0.0);

    const double m_recip = m_massPiston + m_kRodOsc * m_massRod;

    const double engineCycle = (m_engineParams.taktnost == 2) ? 360.0 : 720.0;

    // pressure cycle detection: 360 vs 720
    double maxP = 0.0;
    for (double a : m_pressureAngles)
        if (a > maxP) maxP = a;
    const double pressureCycle = (maxP > 360.0 + 1e-9) ? 720.0 : 360.0;

    // --- ignition shifts (user input) ---
    std::vector<double> ignitionShift(numCyl, 0.0);

    if (m_engineParams.cyl_cycle_phase_deg.size() == numCyl)
    {
        for (size_t c = 0; c < numCyl; ++c)
            ignitionShift[c] = normDeg(m_engineParams.cyl_cycle_phase_deg[c], engineCycle);
    }
    else
    {
        // Жестко: для 4Т без углов вспышки считать "динамику" бессмысленно.
        // Для 2Т можно было бы взять геометрию, но ты просишь именно ввод пользователя.
        throw std::runtime_error("No ignition angles provided: EngineParams.cyl_cycle_phase_deg size mismatch");
    }

    // --- TDC validation (geometric) ---
    const double tolDeg = (m_engineParams.tdc_tolerance_deg > 0.0) ? m_engineParams.tdc_tolerance_deg : 1.0;

    // Find geometric TDC per cylinder: max stroke on grid
    std::vector<double> tdcAngleDeg(numCyl, 0.0);
    for (size_t cyl = 0; cyl < numCyl; ++cyl)
    {
        const auto& s = m_kinematicResults.cylinder_stroke_full[cyl];
        if (s.empty())
            throw std::runtime_error("Kinematic stroke data is empty for cylinder " + std::to_string(cyl + 1));

        size_t bestIdx = 0;
        double bestVal = std::numeric_limits<double>::infinity();

for (size_t i = 0; i < s.size(); ++i)
{
    if (s[i] < bestVal)
    {
        bestVal = s[i];
        bestIdx = i;
    }
}
tdcAngleDeg[cyl] = m_kinematicResults.alpha[bestIdx]; // 0..engineCycle
    }

    // Validate each user ignition angle: must be near geometric TDC modulo 360
    for (size_t cyl = 0; cyl < numCyl; ++cyl)
    {
        const double fire = normDeg(ignitionShift[cyl], engineCycle);

        const double fire360 = normDeg(fire, 360.0);
        const double tdc360  = normDeg(tdcAngleDeg[cyl], 360.0);

        const double dist = minCircularDistDeg(fire360, tdc360, 360.0);
        if (dist > tolDeg)
        {
            throw std::runtime_error(
                "Ignition angle not near geometric TDC for cylinder " + std::to_string(cyl + 1) +
                " (dist=" + std::to_string(dist) + " deg, tol=" + std::to_string(tolDeg) + " deg)"
            );
        }
    }

    // --- main loop ---
    for (size_t i = 0; i < numPoints; ++i)
    {
        if (cancelFlag && cancelFlag->load()) break;

        const double alphaEngine = m_kinematicResults.alpha[i]; // 0..engineCycle
        double torqueSum = 0.0;

        for (size_t cyl = 0; cyl < numCyl; ++cyl)
        {
            // cylinder angle in engine cycle (for force decomposition)
            const double alphaCylEngine = normDeg(alphaEngine + ignitionShift[cyl], engineCycle);

            // cylinder angle for pressure lookup (shifted diagram per cylinder)
            const double alphaCylP = normDeg(alphaEngine + ignitionShift[cyl], pressureCycle);

            const double pressure = interpolatePressure(alphaCylP);
            const double forceGas = (pressure - 101325.0) * m_pistonArea;

            const double accel = m_kinematicResults.cylinder_acceleration_full[cyl][i];
            const double forceInertia = -m_recip * accel;

            const double forceTotal = forceGas + forceInertia;

            const double beta = m_kinematicResults.cylinder_betta_rod[cyl][i] * (M_PI / 180.0);
            const double cb = std::cos(beta);
            const double safeCb = (std::abs(cb) < 1e-12) ? (cb >= 0.0 ? 1e-12 : -1e-12) : cb;

            const double forceRod = forceTotal / safeCb;

            const double alphaRad = alphaCylEngine * (M_PI / 180.0);
            const double forceTan = forceRod * std::sin(alphaRad + beta);
            const double forceRad = forceRod * std::cos(alphaRad + beta);

            const double torque = forceTan * m_engineParams.radcrank;

            results.gas_force[cyl].push_back(forceGas);
            results.inertia_force[cyl].push_back(forceInertia);
            results.total_force[cyl].push_back(forceTotal);
            results.rod_force[cyl].push_back(forceRod);
            results.tangential_force[cyl].push_back(forceTan);
            results.radial_force[cyl].push_back(forceRad);
            results.cylinder_torque[cyl].push_back(torque);

            torqueSum += torque;
        }

        results.total_torque[i] = torqueSum;

        if (progressCallback)
            progressCallback(static_cast<double>(i + 1) / static_cast<double>(numPoints));
    }

    // Backward compatibility: cylinder 0
    if (!results.gas_force.empty())
    {
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