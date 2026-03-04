#pragma once
#include "core/common/common_types.h"
#include <vector>
#include <functional>
#include <atomic>

class DynamicCalculator {
public:
    DynamicCalculator(const EngineParams& engineParams,
                      const CalculationResults& kinematicResults,
                      const std::vector<double>& pressureAngles,
                      const std::vector<double>& pressureValues,
                      double massPiston,
                      double massRod,
                      double kRodOsc,
                      double bore);

    DynamicResults calculate(
        std::function<void(double)> progressCallback = nullptr,
        std::atomic<bool>* cancelFlag = nullptr
    ) const;

private:
    const EngineParams& m_engineParams;
    const CalculationResults& m_kinematicResults;
    const std::vector<double>& m_pressureAngles;
    const std::vector<double>& m_pressureValues;

    double m_massPiston = 0.0;
    double m_massRod = 0.0;
    double m_kRodOsc = 0.0;
    double m_pistonArea = 0.0;

    double interpolatePressure(double alphaNorm) const;

    static double normDeg(double deg, double cycle);
    static double minCircularDistDeg(double a, double b, double cycle);
};