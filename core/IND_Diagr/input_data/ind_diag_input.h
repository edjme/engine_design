#ifndef KINEMATIC_INPUT_H
#define KINEMATIC_INPUT_H

#include <string>
#include "core/common/ind_diag_common_types.h"

class IndInput
{
public:
    // Инициализация конфигурации по умолчанию
    static void initializeDefaultConfig();

    // Чтение параметров из CSV файла
    static bool IndreadCSVConfig(const std::string &filename, IndParams &params);

    // Ручной ввод параметров
    static IndParams IndmanualInput();

    // Вывод параметров на экран
    static void IndprintParams(const IndParams &params);

    // Меню выбора способа загрузки
    static IndParams IndloadParamsWithMenu();

    // Функции для сохранения параметров
    static void IndsaveParamsToCSV(const IndParams &params, const std::string &filename = "");
    static void IndsaveParamsToFormattedText(const IndParams &params, const std::string &filename = "");
    static void IndofferSaveParams(const IndParams &params);

private:
    // Проверка корректности параметров
    static bool IndvalidateParams(const IndParams &params);

    // Создание CSV файла по умолчанию (теперь приватная)
    static void IndcreateDefaultCSV(const std::string &filename = "");

    // Генерация имени файла с датой и временем
    static std::string IndgenerateFilename(const std::string &prefix = "Ind_params/Ind_params");
};

#endif