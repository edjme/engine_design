#ifndef KINEMATIC_INPUT_H
#define KINEMATIC_INPUT_H

#include <string>
#include "core/common/common_types.h"

class KinematicInput
{
public:
    // Инициализация конфигурации по умолчанию
    static void initializeDefaultConfig();

    // Чтение параметров из CSV файла
    static bool readCSVConfig(const std::string &filename, EngineParams &params);

    // Ручной ввод параметров
    static EngineParams manualInput();

    // Вывод параметров на экран
    static void printParams(const EngineParams &params);

    // Меню выбора способа загрузки
    static EngineParams loadParamsWithMenu();

    // Функции для сохранения параметров
    static void saveParamsToCSV(const EngineParams &params, const std::string &filename = "");
    static void saveParamsToFormattedText(const EngineParams &params, const std::string &filename = "");
    static void offerSaveParams(const EngineParams &params);

private:
    // Проверка корректности параметров
    static bool validateParams(const EngineParams &params);

    // Создание CSV файла по умолчанию (теперь приватная)
    static void createDefaultCSV(const std::string &filename = "");

    // Генерация имени файла с датой и временем
    static std::string generateFilename(const std::string &prefix = "Kinematic_params/kinematic_params");

    // Функции для разных типов КШМ
    static EngineParams inputAxialKSM();
    static EngineParams inputDezaxialKSM();
    static EngineParams inputVShapedKSM();
    static EngineParams inputVShapedKSMwithDEZAXIAL();
    static EngineParams inputVShapedWithAttachedRodKSM();
    static EngineParams inputVShapedWithAttachedRodKSMwithDEZAXIAL();
};

#endif