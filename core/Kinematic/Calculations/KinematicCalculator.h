#pragma once
#include "common/common_types.h"
#include "models/KSMModel.h"
#include <memory>
#include <vector>
#include <functional>
#include <atomic>

class KinematicCalculator {
public:
    explicit KinematicCalculator(const EngineParams& params);

    // Основной метод расчёта – константный, принимает колбэк и флаг отмены
    CalculationResults calculateAll(
        std::function<void(double)> progressCallback = nullptr,
        std::atomic<bool>* cancelFlag = nullptr
    ) const;

private:
    EngineParams m_params;
    std::unique_ptr<KSMModel> m_model;

    std::vector<double> generateAlphaGrid() const;

    // Для кинематики это геометрические фазы КВ (cyl_geom_phase_deg / legacy cyl_phase_deg)
    std::vector<double> calculateFiringAngles() const;

    void calculateCylinder(
        int cylinderIndex,
        const std::vector<double>& alpha,
        CalculationResults& results,
        const std::vector<double>& phaseShifts,
        std::atomic<bool>* cancelFlag
    ) const;
};