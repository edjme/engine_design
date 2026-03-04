#pragma once

#include <string>
#include <vector>
#include "core/common/common_types.h"

// Доп. инфо для шапки экспорта (чтобы не терять контекст динамики)
struct DynamicExportInfo
{
    std::string pressure_file;   // путь/имя файла P(α)
    std::string pressure_unit;   // BAR/MPA/...
    double mass_piston = 0.0;    // кг
    double mass_rod = 0.0;       // кг
    double k_rod_osc = 0.0;      // доля
    double bore = 0.0;           // м
};

class DynamicOutput
{
public:
    // CSV "табличный" (wide): alpha + total_torque + далее блоки по цилиндрам (как в кинематике)
    static bool saveToCSVWide(const std::vector<double>& alpha,
                              const DynamicResults& results,
                              const EngineParams& params,
                              const DynamicExportInfo& info,
                              const std::string& filename);

    // CSV "инженерный" (long): каждая строка = (alpha, cyl, ...), удобно для Python/Matlab/PowerQuery
    static bool saveToCSVLong(const std::vector<double>& alpha,
                              const DynamicResults& results,
                              const EngineParams& params,
                              const DynamicExportInfo& info,
                              const std::string& filename);

    // TXT форматированный: параметры + сводка + таблица
    static bool saveToFormattedText(const std::vector<double>& alpha,
                                    const DynamicResults& results,
                                    const EngineParams& params,
                                    const DynamicExportInfo& info,
                                    const std::string& filename);

private:
    static bool canWriteToFile(const std::string& filename);

    // Шапка (комментариями), чтобы Excel/читалки не путались
    static void writeHeaderCSV(std::ofstream& file,
                               const EngineParams& params,
                               const DynamicExportInfo& info);

    static void writeHeaderText(std::ofstream& file,
                                const EngineParams& params,
                                const DynamicExportInfo& info);

    // Сводка по одному вектору: min/max/avg + углы
    static void writeVectorStats(std::ofstream& file,
                                 const std::string& title,
                                 const std::vector<double>& x,
                                 const std::vector<double>& alpha,
                                 const std::string& unit);
};