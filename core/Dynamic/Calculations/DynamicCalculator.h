#pragma once
#include "core/common/common_types.h"
#include <vector>
#include <functional>

class DynamicCalculator {
public:
    // Конструктор принимает параметры двигателя, результаты кинематики и данные давления
    DynamicCalculator(const EngineParams& engineParams,
                      const CalculationResults& kinematicResults,
                      const std::vector<double>& pressureAngles,
                      const std::vector<double>& pressureValues,
                      double massPiston,    // кг
                      double massRod,       // кг
                      double kRodOsc, // доля возвратно-поступательной массы шатуна
                      double bore);      

    // Основной метод расчёта
    DynamicResults calculate();

    // Прогресс (опционально)
    using ProgressCallback = std::function<void(double progress, bool& cancelled)>;
    void setProgressCallback(ProgressCallback callback) { m_callback = callback; }
    bool isCancelled() const { return m_cancelled; }
    void cancel() { m_cancelled = true; }

private:
    const EngineParams& m_engineParams;
    const CalculationResults& m_kinematicResults;
    std::vector<double> m_pressureAngles;
    std::vector<double> m_pressureValues;
    double m_massPiston;
    double m_massRod;
    double m_kRodOsc;

    ProgressCallback m_callback;
    bool m_cancelled = false;

    // Интерполяция давления для заданного угла (с учётом тактности)
    double interpolatePressure(double alpha) const;

    // Площадь поршня (вычисляется по радиусу кривошипа? или нужен диаметр? Пока принимаем)
    double m_pistonArea; // будет вычислена в конструкторе
};