#ifndef KINEMATIC_OUTPUT_H
#define KINEMATIC_OUTPUT_H

#include <string>
#include "common/common_types.h"

class KinematicOutput
{
public:
    // Генерация имени файла с датой и временем
    static std::string generateFilename(const std::string &prefix = "ksm_results");

    // Сохранение результатов в CSV файл
    static bool saveToCSV(const CalculationResults &results,
                          const EngineParams &params,
                          const std::string &filename = "");

    // Сохранение результатов в текстовый файл с форматированием
    static bool saveToFormattedText(const CalculationResults &results,
                                    const EngineParams &params,
                                    const std::string &filename = "");

    // Сохранение только основных параметров (укороченная версия)
    static bool saveSummary(const CalculationResults &results,
                            const EngineParams &params,
                            const std::string &filename = "");

    // Показать меню выбора формата экспорта
    static void exportWithMenu(const CalculationResults &results,
                               const EngineParams &params);

private:
    // Вспомогательная функция для записи заголовка
    static void writeHeader(std::ofstream &file, const EngineParams &params);

    // Проверка возможности записи в файл
    static bool canWriteToFile(const std::string &filename);
};

#endif