#pragma once
#include "common/common_types.h"
#include <memory>
#include <vector>
#include <cmath>

// Перечисление типов КШМ (дублируем из MainFrame.h для независимости)
enum class KSMType {
    Axial = 0,
    Deaxial,
    VShaped,
    VShapedDeaxial,
    VShapedAttached,
    VShapedAttachedDeaxial
};

// Структура для результатов одного цилиндра
struct CylinderResults {
    std::vector<double> stroke_full;
    std::vector<double> stroke1;
    std::vector<double> stroke2;
    std::vector<double> velocity_full;
    std::vector<double> velocity1;
    std::vector<double> velocity2;
    std::vector<double> acceleration_full;
    std::vector<double> acceleration1;
    std::vector<double> acceleration2;
    std::vector<double> betta_rod;
    std::vector<double> omega_rod;
    std::vector<double> eps_rod;
    
    // Для бокового цилиндра (если есть)
    std::unique_ptr<CylinderResults> side;
    
    // Конструктор
    CylinderResults() : side(nullptr) {}
    
    // Очистка всех векторов
    void clear() {
        stroke_full.clear();
        stroke1.clear();
        stroke2.clear();
        velocity_full.clear();
        velocity1.clear();
        velocity2.clear();
        acceleration_full.clear();
        acceleration1.clear();
        acceleration2.clear();
        betta_rod.clear();
        omega_rod.clear();
        eps_rod.clear();
        if (side) {
            side->clear();
        }
    }
    
    // Резервирование памяти
    void reserve(size_t n) {
        stroke_full.reserve(n);
        stroke1.reserve(n);
        stroke2.reserve(n);
        velocity_full.reserve(n);
        velocity1.reserve(n);
        velocity2.reserve(n);
        acceleration_full.reserve(n);
        acceleration1.reserve(n);
        acceleration2.reserve(n);
        betta_rod.reserve(n);
        omega_rod.reserve(n);
        eps_rod.reserve(n);
    }
    
    // Создать боковой цилиндр при необходимости
    void ensureSide() {
        if (!side) {
            side = std::make_unique<CylinderResults>();
        }
    }
};

class KSMModel {
public:
    virtual ~KSMModel() = default;
    
    // Основной метод расчёта для одного цилиндра
    // results - структура для заполнения результатами
    // alpha - вектор углов поворота (в градусах)
    // phaseShift - фазовый сдвиг для данного цилиндра (в градусах)
    virtual void calculate(CylinderResults& results, 
                           const std::vector<double>& alpha,
                           double phaseShift) const = 0;
    
    // Возвращает true, если у этой схемы есть боковой цилиндр
    virtual bool hasSideCylinder() const = 0;
    
    // Возвращает количество цилиндров в одном блоке (для V-образных - это количество рядов)
    virtual int cylindersPerBlock() const = 0;
    
    // Фабричный метод для создания модели по типу КШМ
    static std::unique_ptr<KSMModel> create(KSMType type, 
                                            const EngineParams& params);
    
    // Получить тип модели
    KSMType getType() const { return m_type; }
    
protected:
    KSMModel(KSMType type, const EngineParams& params) 
        : m_type(type), m_params(params) {}
    
    KSMType m_type;
    EngineParams m_params;
    
    // Константы для пересчёта
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double DEG_TO_RAD = PI / 180.0;
    static constexpr double RAD_TO_DEG = 180.0 / PI;
    
    // Вспомогательные функции для производных классов
    double deg2rad(double deg) const { return deg * DEG_TO_RAD; }
    double rad2deg(double rad) const { return rad * RAD_TO_DEG; }

    // Нормализация угла по циклу (360/720) с корректной обработкой отрицательных значений
double normalizeAngleDeg(double deg) const {
    const double cycle = (m_params.end_alpha > 0.0)
        ? m_params.end_alpha
        : ((m_params.taktnost == 2) ? 360.0 : 720.0);

    double x = std::fmod(deg, cycle);
    if (x < 0.0) x += cycle;
    return x;
}
};