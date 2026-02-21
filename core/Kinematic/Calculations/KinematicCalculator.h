#pragma once
#include "common/common_types.h"
#include "models/KSMModel.h"
#include <memory>
#include <vector>
#include <functional>

class KinematicCalculator {
public:
    // Конструктор принимает параметры двигателя
    explicit KinematicCalculator(const EngineParams& params);
    
    // Основной метод расчёта всех цилиндров
    CalculationResults calculateAll();
    
    // Включить/выключить многопоточный расчёт
    void setParallel(bool enable) { m_parallel = enable; }
    
    // Получить информацию о прогрессе (для прогресс-бара)
    double getProgress() const { return m_progress; }
    
    // Отменить текущий расчёт (для долгих операций)
    void cancel() { m_cancelled = true; }

    using ProgressCallback = std::function<void(double progress, bool& cancelled)>;
    void setProgressCallback(ProgressCallback callback);

    bool isCancelled() const { return m_cancelled; }
    
private:
    EngineParams m_params;
    std::unique_ptr<KSMModel> m_model;
    bool m_parallel = false;
    mutable double m_progress = 0.0;
    mutable bool m_cancelled = false;

    ProgressCallback m_callback;
    
    // Генерация сетки углов
    std::vector<double> generateAlphaGrid() const;
    
    // Расчёт углов чередования вспышек
    std::vector<double> calculateFiringAngles() const;
    
    // Расчёт одного цилиндра (для последовательного режима)
    void calculateCylinder(int cylinderIndex, 
                          const std::vector<double>& alpha,
                          CalculationResults& results) const;
    
    // Расчёт одного цилиндра в отдельном потоке (для параллельного режима)
    static void calculateCylinderThread(int cylinderIndex,
                                       const EngineParams& params,
                                       const std::unique_ptr<KSMModel>& model,
                                       const std::vector<double>& alpha,
                                       CalculationResults& results,
                                       std::atomic<int>& completed,
                                       std::atomic<bool>& cancelled);
};