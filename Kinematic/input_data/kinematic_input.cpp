#include "kinematic_input.h"
#include "path_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <ctime>

using namespace std;

// НОВАЯ ФУНКЦИЯ: Проверка и создание файла по умолчанию при запуске
void KinematicInput::initializeDefaultConfig()
{
    string defaultFile = PathManager::getInputDirectory() + "/kinematic_input.csv";

#ifdef _WIN32
    wstring wide_filename = PathManager::utf8ToWide(defaultFile);
    ifstream testFile(wide_filename.c_str());
#else
    ifstream testFile(defaultFile);
#endif

    // Если файл не существует, создаем его
    if (!testFile.is_open())
    {
        cout << "Файл конфигурации по умолчанию не найден. Создаю..." << endl;
        createDefaultCSV();
    }
    else
    {
        testFile.close();
        cout << "Файл конфигурации по умолчанию найден." << endl;
    }
}

// Функция проверки корректности параметров
bool KinematicInput::validateParams(const EngineParams &params)
{
    if (params.step_alpha <= 0 || params.step_alpha > 360)
    {
        cerr << "Ошибка: Шаг угла должен быть в диапазоне (0, 360] град." << endl;
        return false;
    }
    if (params.end_alpha <= 0 || params.end_alpha > 720)
    {
        cerr << "Ошибка: Предельный угол должен быть в диапазоне (0, 720] град." << endl;
        return false;
    }
    if (params.radcrank <= 0)
    {
        cerr << "Ошибка: Радиус кривошипа должен быть положительным." << endl;
        return false;
    }
    if (params.lyambda <= 0 || params.lyambda >= 1)
    {
        cerr << "Ошибка: Геометрическая характеристика КШМ λ должна быть в диапазоне (0, 1)." << endl;
        return false;
    }
    if (params.n <= 0)
    {
        cerr << "Ошибка: Частота вращения должна быть положительной." << endl;
        return false;
    }
    if (params.gamma < 0 || params.gamma > 180)
    {
        cerr << "Ошибка: Угол развала должен быть в диапазоне [0, 180] град." << endl;
        return false;
    }
    if (params.gammaPric < 0 || params.gammaPric > 180)
    {
        cerr << "Ошибка: Угол прицепного шатуна должен быть в диапазоне [0, 180] град." << endl;
        return false;
    }
    if (params.radcrank1 < 0)
    {
        cerr << "Ошибка: Радиус кривошипа прицепного шатуна не может быть отрицательным." << endl;
        return false;
    }
    if (params.lengthRod1 < 0)
    {
        cerr << "Ошибка: Длина прицепного шатуна не может быть отрицательной." << endl;
        return false;
    }

    return true;
}

// Функция генерации имени файла с датой и временем
string KinematicInput::generateFilename(const string &prefix)
{
    time_t now = time(0);
    tm *localTime = localtime(&now);

    stringstream filename;
    filename << prefix << "_"
             << setw(2) << setfill('0') << localTime->tm_hour << "-"
             << setw(2) << setfill('0') << localTime->tm_min << "-"
             << setw(2) << setfill('0') << localTime->tm_sec << "_"
             << setw(2) << setfill('0') << localTime->tm_mday << "."
             << setw(2) << setfill('0') << (localTime->tm_mon + 1) << "."
             << (1900 + localTime->tm_year);

    return filename.str();
}

// Функция сохранения параметров в CSV
void KinematicInput::saveParamsToCSV(const EngineParams &params, const string &filename)
{
    // Создаем директории если их нет
    if (!PathManager::ensureDirectoriesExist())
    {
        cerr << "Ошибка: не удалось создать рабочие директории" << endl;
        return;
    }

    string outputFilename = filename.empty() ? PathManager::getInputDirectory() + "/kinematic_params.csv" : filename;

#ifdef _WIN32
    // Для Windows используем широкие символы только для пути
    wstring wide_filename = PathManager::utf8ToWide(outputFilename);
    ofstream file(wide_filename.c_str(), ios::binary); // Используем ofstream и binary mode
#else
    ofstream file(outputFilename, ios::binary); // Используем binary mode
#endif

    if (file.is_open())
    {
        // Записываем BOM для UTF-8 как байты
        file << "\xEF\xBB\xBF";

        file << "параметр;значение;единица измерения;описание\n";
        file << "step_alpha;" << params.step_alpha << ";град.;Шаг угла поворота кривошипа\n";
        file << "end_alpha;" << params.end_alpha << ";град.;Предельный угол поворота кривошипа\n";
        file << "radcrank;" << params.radcrank << ";м;Радиус кривошипа\n";
        file << "lyambda;" << params.lyambda << "-;Геометрическая характеристика КШМ (λ)\n";
        file << "n;" << params.n << ";об/мин;Частота вращения коленвала\n";
        file << "gamma;" << params.gamma << ";град.;Угол развала цилиндров\n";
        file << "gammaPric;" << params.gammaPric << ";град.;Угол прицепного шатуна\n";
        file << "dezaxial;" << params.dezaxial << ";м;Дезаксиал\n";
        file << "radcrank1;" << params.radcrank1 << ";м;Радиус кривошипа прицепного шатуна\n";
        file << "lengthRod1;" << params.lengthRod1 << ";м;Длина прицепного шатуна\n";
        file.close();
        cout << "Параметры сохранены в CSV файл: " << outputFilename << endl;
    }
    else
    {
        cerr << "Ошибка создания CSV файла: " << outputFilename << endl;
    }
}

// Функция сохранения параметров в форматированный текстовый файл
void KinematicInput::saveParamsToFormattedText(const EngineParams &params, const string &filename)
{
    // Создаем директории если их нет
    if (!PathManager::ensureDirectoriesExist())
    {
        cerr << "Ошибка: не удалось создать рабочие директории" << endl;
        return;
    }

    string outputFilename = filename.empty() ? PathManager::getInputDirectory() + "/kinematic_input.txt" : filename;

#ifdef _WIN32
    // Для Windows используем широкие символы
    wstring wide_filename = PathManager::utf8ToWide(outputFilename);
    wofstream file(wide_filename.c_str()); // Используем c_str()
#else
    ofstream file(outputFilename);
#endif

    if (file.is_open())
    {
        time_t now = time(0);

        file << "=========================================\n";
        file << "       ПАРАМЕТРЫ РАСЧЕТА КШМ\n";
        file << "=========================================\n";
        file << "Дата создания: " << ctime(&now);
        file << "=========================================\n";
        file << "Шаг угла α: " << params.step_alpha << " град.\n";
        file << "Предел угла α: " << params.end_alpha << " град.\n";
        file << "Радиус кривошипа: " << params.radcrank << " м\n";
        file << "λ: " << params.lyambda << "\n";
        file << "Частота вращения: " << params.n << " об/мин\n";
        if (params.gamma != 0)
        {
            file << "Угол развала: " << params.gamma << " град.\n";
        }
        if (params.gammaPric != 0)
        {
            file << "Угол прицепного шатуна: " << params.gammaPric << " град.\n";
        }
        if (params.dezaxial != 0)
        {
            file << "Дезаксиал: " << params.dezaxial << " м\n";
        }
        if (params.radcrank1 != 0)
        {
            file << "Радиус кривошипа прицепного шатуна: " << params.radcrank1 << " м\n";
        }
        if (params.lengthRod1 != 0)
        {
            file << "Длина прицепного шатуна: " << params.lengthRod1 << " м\n";
        }
        file << "=========================================\n";
        file.close();
        cout << "Параметры сохранены в текстовый файл: " << outputFilename << endl;
    }
    else
    {
        cerr << "Ошибка создания текстового файла: " << outputFilename << endl;
    }
}

// Функция предложения сохранения параметров
void KinematicInput::offerSaveParams(const EngineParams &params)
{
    char choice;
    cout << "\nХотите сохранить введенные параметры в файл? (1 - ДА / 2 - НЕТ): ";
    cin >> choice;

    if (choice == '1')
    {
        int saveChoice;
        cout << "\n=== СОХРАНЕНИЕ ПАРАМЕТРОВ ===\n";
        cout << "1 - Сохранить в CSV\n";
        cout << "2 - Сохранить в текстовый файл\n";
        cout << "3 - Сохранить в оба формата\n";
        cout << "0 - Не сохранять\n";
        cout << "Выберите действие: ";
        cin >> saveChoice;

        string filename;
        switch (saveChoice)
        {
        case 1:
            cout << "Использовать автоматическое имя для CSV? (1 - ДА / 2 - НЕТ): ";
            cin >> choice;
            if (choice == '2')
            {
                cout << "Введите имя CSV файла: ";
                cin >> filename;
            }
            saveParamsToCSV(params, filename);
            break;
        case 2:
            cout << "Использовать автоматическое имя для текстового файла? (1 - ДА / 2 - НЕТ): ";
            cin >> choice;
            if (choice == '1')
            {
                cout << "Введите имя текстового файла: ";
                cin >> filename;
            }
            saveParamsToFormattedText(params, filename);
            break;
        case 3:
            saveParamsToCSV(params);
            saveParamsToFormattedText(params);
            break;
        case 0:
            cout << "Параметры не сохранены.\n";
            break;
        default:
            cout << "Неверный выбор. Параметры не сохранены.\n";
            break;
        }
    }
}

void KinematicInput::createDefaultCSV(const string &filename)
{
    // Создаем директории если их нет
    if (!PathManager::ensureDirectoriesExist())
    {
        cerr << "Ошибка: не удалось создать рабочие директории" << endl;
        return;
    }

    string fullFilename = filename.empty() ? PathManager::getInputDirectory() + "/kinematic_input.csv" : filename;

#ifdef _WIN32
    // Для Windows используем широкие символы только для пути
    wstring wide_filename = PathManager::utf8ToWide(fullFilename);
    ofstream file(wide_filename.c_str(), ios::binary);
#else
    ofstream file(fullFilename, ios::binary);
#endif

    if (file.is_open())
    {
        // Записываем BOM для UTF-8 как байты
        file << "\xEF\xBB\xBF";

        file << "параметр;значение;единица измерения;описание\n";
        file << "step_alpha;1.0;град.;Шаг угла поворота кривошипа\n";
        file << "end_alpha;360.0;град.;Предельный угол поворота кривошипа\n";
        file << "radcrank;0.020;м;Радиус кривошипа\n";
        file << "lyambda;0.3;-;Геометрическая характеристика КШМ (λ)\n";
        file << "n;4800.0;об/мин;Частота вращения коленвала\n";
        file << "gamma;0.0;град.;Угол развала цилиндров\n";
        file << "gammaPric;0.0;град.;Угол прицепного шатуна\n";
        file << "dezaxial;0.0;м;Дезаксиал\n";
        file << "radcrank1;0.0;м;Радиус кривошипа прицепного шатуна\n";
        file << "lengthRod1;0.0;м;Длина прицепного шатуна\n";
        file.close();
        cout << "Создан CSV файл конфигурации по умолчанию: " << fullFilename << endl;
    }
    else
    {
        cerr << "Ошибка создания CSV файла по умолчанию: " << fullFilename << endl;
    }
}

bool KinematicInput::readCSVConfig(const string &filename, EngineParams &params)
{
    string fullFilename = filename;

    // Если имя файла не содержит путь, добавляем путь к директории ввода
    if (filename.find('/') == string::npos && filename.find('\\') == string::npos)
    {
        fullFilename = PathManager::getInputDirectory() + "/" + filename;
    }

#ifdef _WIN32
    // Для Windows используем широкие символы
    wstring wide_filename = PathManager::utf8ToWide(fullFilename);
    wifstream file(wide_filename.c_str());

    if (!file.is_open())
    {
        cerr << "Ошибка открытия CSV файла: " << fullFilename << endl;
        return false;
    }

    wstring line;
    int lineNum = 0;

    // Читаем файл построчно с использованием широких символов
    while (getline(file, line))
    {
        lineNum++;
        if (lineNum == 1 || line.empty())
            continue;

        wstringstream ss(line);
        wstring token;
        vector<wstring> tokens;

        while (getline(ss, token, L';'))
        {
            tokens.push_back(token);
        }

        if (tokens.size() >= 2)
        {
            wstring paramName = tokens[0];
            wstring valueStr = tokens[1];

            try
            {
                // Конвертируем широкую строку в обычную для stod
                string narrowValueStr = PathManager::wideToUtf8(valueStr);
                double value = stod(narrowValueStr);

                // Конвертируем имя параметра для сравнения
                string narrowParamName = PathManager::wideToUtf8(paramName);

                if (narrowParamName == "step_alpha")
                    params.step_alpha = value;
                else if (narrowParamName == "end_alpha")
                    params.end_alpha = value;
                else if (narrowParamName == "radcrank")
                    params.radcrank = value;
                else if (narrowParamName == "lyambda")
                    params.lyambda = value;
                else if (narrowParamName == "n")
                    params.n = value;
                else if (narrowParamName == "gamma")
                    params.gamma = value;
                else if (narrowParamName == "gammaPric")
                    params.gammaPric = value;
                else if (narrowParamName == "dezaxial")
                    params.dezaxial = value;
                else if (narrowParamName == "radcrank1")
                    params.radcrank1 = value;
                else if (narrowParamName == "lengthRod1")
                    params.lengthRod1 = value;
            }
            catch (const exception &e)
            {
                cerr << "Ошибка парсинга значения в строке " << lineNum << ": " << e.what() << endl;
            }
        }
    }

    file.close();
    return true;

#else
    // Linux версия - использует обычные строки
    ifstream file(fullFilename);
    if (!file.is_open())
    {
        cerr << "Ошибка открытия CSV файла: " << fullFilename << endl;
        return false;
    }

    string line;
    int lineNum = 0;

    while (getline(file, line))
    {
        lineNum++;
        if (lineNum == 1 || line.empty())
            continue;

        stringstream ss(line);
        string token;
        vector<string> tokens;

        while (getline(ss, token, ';'))
        {
            tokens.push_back(token);
        }

        if (tokens.size() >= 2)
        {
            string paramName = tokens[0];
            string valueStr = tokens[1];

            try
            {
                double value = stod(valueStr);

                if (paramName == "step_alpha")
                    params.step_alpha = value;
                else if (paramName == "end_alpha")
                    params.end_alpha = value;
                else if (paramName == "radcrank")
                    params.radcrank = value;
                else if (paramName == "lyambda")
                    params.lyambda = value;
                else if (paramName == "n")
                    params.n = value;
                else if (paramName == "gamma")
                    params.gamma = value;
                else if (paramName == "gammaPric")
                    params.gammaPric = value;
                else if (paramName == "dezaxial")
                    params.dezaxial = value;
                else if (paramName == "radcrank1")
                    params.radcrank1 = value;
                else if (paramName == "lengthRod1")
                    params.lengthRod1 = value;
            }
            catch (const exception &e)
            {
                cerr << "Ошибка парсинга значения в строке " << lineNum << ": " << e.what() << endl;
            }
        }
    }

    file.close();
    return true;
#endif
}

// Функции для разных типов КШМ
EngineParams KinematicInput::inputAxialKSM()
{
    EngineParams params;
    cout << "\n=== АКСИАЛЬНЫЙ КШМ ===\n";
    cout << "Введите шаг альфа (град.): ";
    cin >> params.step_alpha;
    cout << "Введите предел альфа (град.): ";
    cin >> params.end_alpha;
    cout << "Введите радиус кривошипа (м): ";
    cin >> params.radcrank;
    cout << "Введите геометрическую характеристику КШМ: ";
    cin >> params.lyambda;
    cout << "Введите частоту вращения (об/мин): ";
    cin >> params.n;
    // Для аксиального КШМ остальные параметры = 0
    params.gamma = 0.0;
    params.gammaPric = 0.0;
    params.dezaxial = 0.0;
    params.radcrank1 = 0.0;
    params.lengthRod1 = 0.0;

    return params;
}

EngineParams KinematicInput::inputDezaxialKSM()
{
    EngineParams params;
    cout << "\n=== ДЕЗАКСИАЛЬНЫЙ КШМ ===\n";
    cout << "Введите шаг альфа (град.): ";
    cin >> params.step_alpha;
    cout << "Введите предел альфа (град.): ";
    cin >> params.end_alpha;
    cout << "Введите радиус кривошипа (м): ";
    cin >> params.radcrank;
    cout << "Введите геометрическую характеристику КШМ: ";
    cin >> params.lyambda;
    cout << "Введите частоту вращения (об/мин): ";
    cin >> params.n;
    cout << "Введите дезаксиал (м): ";
    cin >> params.dezaxial;
    // Для дезаксиального КШМ остальные параметры = 0
    params.gamma = 0.0;
    params.gammaPric = 0.0;
    params.radcrank1 = 0.0;
    params.lengthRod1 = 0.0;

    return params;
}

EngineParams KinematicInput::inputVShapedKSM()
{
    EngineParams params;
    cout << "\n=== V-ОБРАЗНЫЙ КШМ С РЯДОМ СИДЯЩИМИ ШАТУНАМИ ===\n";
    cout << "Введите шаг альфа (град.): ";
    cin >> params.step_alpha;
    cout << "Введите предел альфа (град.): ";
    cin >> params.end_alpha;
    cout << "Введите радиус кривошипа (м): ";
    cin >> params.radcrank;
    cout << "Введите геометрическую характеристику КШМ: ";
    cin >> params.lyambda;
    cout << "Введите частоту вращения (об/мин): ";
    cin >> params.n;
    cout << "Введите угол развала (град.): ";
    cin >> params.gamma;
    // Для V-образного КШМ остальные параметры = 0
    params.gammaPric = 0.0;
    params.radcrank1 = 0.0;
    params.lengthRod1 = 0.0;
    params.dezaxial = 0.0;

    return params;
}

EngineParams KinematicInput::inputVShapedKSMwithDEZAXIAL()
{
    EngineParams params;
    cout << "\n=== V-ОБРАЗНЫЙ КШМ С РЯДОМ СИДЯЩИМИ ШАТУНАМИ ===\n";
    cout << "Введите шаг альфа (град.): ";
    cin >> params.step_alpha;
    cout << "Введите предел альфа (град.): ";
    cin >> params.end_alpha;
    cout << "Введите радиус кривошипа (м): ";
    cin >> params.radcrank;
    cout << "Введите геометрическую характеристику КШМ: ";
    cin >> params.lyambda;
    cout << "Введите частоту вращения (об/мин): ";
    cin >> params.n;
    cout << "Введите угол развала (град.): ";
    cin >> params.gamma;
    cout << "Введите дезаксиал (м): ";
    cin >> params.dezaxial;
    // Для V-образного КШМ остальные параметры = 0
    params.gammaPric = 0.0;
    params.radcrank1 = 0.0;
    params.lengthRod1 = 0.0;

    return params;
}

EngineParams KinematicInput::inputVShapedWithAttachedRodKSM()
{
    EngineParams params;
    cout << "\n=== V-ОБРАЗНЫЙ КШМ С ПРИЦЕПНЫМ ШАТУНОМ ===\n";
    cout << "Введите шаг альфа (град.): ";
    cin >> params.step_alpha;
    cout << "Введите предел альфа (град.): ";
    cin >> params.end_alpha;
    cout << "Введите радиус кривошипа (м): ";
    cin >> params.radcrank;
    cout << "Введите геометрическую характеристику КШМ: ";
    cin >> params.lyambda;
    cout << "Введите частоту вращения (об/мин): ";
    cin >> params.n;
    cout << "Введите угол развала (град.): ";
    cin >> params.gamma;
    cout << "Введите угол прицепного шатуна (град.): ";
    cin >> params.gammaPric;
    cout << "Введите радиус кривошипа прицепного шатуна (м): ";
    cin >> params.radcrank1;
    cout << "Введите длину прицепного шатуна (м): ";
    cin >> params.lengthRod1;
    // Для этого типа дезаксиал = 0
    params.dezaxial = 0.0;

    return params;
}

EngineParams KinematicInput::inputVShapedWithAttachedRodKSMwithDEZAXIAL()
{
    EngineParams params;
    cout << "\n=== V-ОБРАЗНЫЙ КШМ С ПРИЦЕПНЫМ ШАТУНОМ ===\n";
    cout << "Введите шаг альфа (град.): ";
    cin >> params.step_alpha;
    cout << "Введите предел альфа (град.): ";
    cin >> params.end_alpha;
    cout << "Введите радиус кривошипа (м): ";
    cin >> params.radcrank;
    cout << "Введите геометрическую характеристику КШМ: ";
    cin >> params.lyambda;
    cout << "Введите частоту вращения (об/мин): ";
    cin >> params.n;
    cout << "Введите угол развала (град.): ";
    cin >> params.gamma;
    cout << "Введите угол прицепного шатуна (град.): ";
    cin >> params.gammaPric;
    cout << "Введите радиус кривошипа прицепного шатуна (м): ";
    cin >> params.radcrank1;
    cout << "Введите длину прицепного шатуна (м): ";
    cin >> params.lengthRod1;
    cout << "Введите дезаксиал (м): ";
    cin >> params.dezaxial;

    return params;
}

EngineParams KinematicInput::manualInput()
{
    int ksmType;
    cout << "\n=== ВЫБОР ТИПА КШМ ===\n";
    cout << "1 - Аксиальный КШМ\n";
    cout << "2 - Дезаксиальный КШМ\n";
    cout << "3 - V-образный с рядом сидящими шатунами\n";
    cout << "4 - V-образный с прицепным шатуном\n";
    cout << "5 - V-образный с рядом сидящими шатунами дезаксиальный\n";
    cout << "6 - V-образный с прицепным шатуном дезаксиальный\n";

    cout << "Выберите тип КШМ: ";
    cin >> ksmType;

    EngineParams params;

    switch (ksmType)
    {
    case 1:
        params = inputAxialKSM();
        break;
    case 2:
        params = inputDezaxialKSM();
        break;
    case 3:
        params = inputVShapedKSM();
        break;
    case 4:
        params = inputVShapedWithAttachedRodKSM();
        break;
    case 5:
        params = inputVShapedKSMwithDEZAXIAL();
        break;
    case 6:
        params = inputVShapedWithAttachedRodKSMwithDEZAXIAL();
        break;
    default:
        cout << "Неверный выбор. Используется аксиальный КШМ по умолчанию.\n";
        params = inputAxialKSM();
        break;
    }

    // Предлагаем сохранить параметры
    offerSaveParams(params);

    return params;
}

void KinematicInput::printParams(const EngineParams &params)
{
    cout << "\n=== ЗАГРУЖЕННЫЕ ПАРАМЕТРЫ ===\n";
    cout << "Шаг альфа: " << params.step_alpha << " град.\n";
    cout << "Предел альфа: " << params.end_alpha << " град.\n";
    cout << "Радиус кривошипа: " << params.radcrank << " м\n";
    cout << "Геометрическая характеристика: " << params.lyambda << "\n";
    cout << "Частота вращения: " << params.n << " об/мин\n";
    // Выводим только ненулевые параметры
    if (params.gamma != 0)
    {
        cout << "Угол развала: " << params.gamma << " град.\n";
    }
    if (params.gammaPric != 0)
    {
        cout << "Угол прицепного шатуна: " << params.gammaPric << " град.\n";
    }
    if (params.dezaxial != 0)
    {
        cout << "Дезаксиал: " << params.dezaxial << " м\n";
    }
    if (params.radcrank1 != 0)
    {
        cout << "Радиус кривошипа прицепного шатуна: " << params.radcrank1 << " м\n";
    }
    if (params.lengthRod1 != 0)
    {
        cout << "Длина прицепного шатуна: " << params.lengthRod1 << " м\n";
    }
    cout << "================================\n\n";
}

EngineParams KinematicInput::loadParamsWithMenu()
{
    // Автоматически инициализируем конфигурацию при запуске
    initializeDefaultConfig();

    EngineParams params;
    string filename;
    int choice;

    cout << "=== ЗАГРУЗКА ПАРАМЕТРОВ ===\n";
    cout << "1 - Использовать файл по умолчанию (kinematic_input.csv)\n";
    cout << "2 - Загрузить другой CSV файл\n";
    cout << "3 - Ручной ввод параметров\n";
    cout << "Выберите действие: ";
    cin >> choice;

    bool paramsValid = false;

    while (!paramsValid)
    {
        switch (choice)
        {
        case 1:
            if (!readCSVConfig("kinematic_input.csv", params))
            {
                cout << "Ошибка загрузки файла по умолчанию. Использую ручной ввод.\n";
                params = manualInput();
            }
            break;
        case 2:
            cout << "Введите имя CSV файла: ";
            cin >> filename;
            if (!readCSVConfig(filename, params))
            {
                cout << "Файл не найден. Использую ручной ввод.\n";
                params = manualInput();
            }
            break;
        case 3:
            params = manualInput();
            break;
        default:
            cout << "Неверный выбор. Использую файл по умолчанию.\n";
            if (!readCSVConfig("kinematic_input.csv", params))
            {
                cout << "Ошибка загрузки файла по умолчанию. Использую значения по умолчанию.\n";
                // Создаем параметры по умолчанию
                params.step_alpha = 1.0;
                params.end_alpha = 360.0;
                params.radcrank = 0.020;
                params.lyambda = 0.3;
                params.n = 4800.0;
                params.gamma = 0.0;
                params.gammaPric = 0.0;
                params.dezaxial = 0.0;
                params.radcrank1 = 0.0;
                params.lengthRod1 = 0.0;
            }
            break;
        }

        // Проверяем корректность параметров
        paramsValid = validateParams(params);
        if (!paramsValid)
        {
            cout << "\nОбнаружены ошибки в параметрах. Пожалуйста, выберите другой способ загрузки:\n";
            cout << "1 - Использовать файл по умолчанию\n";
            cout << "2 - Загрузить другой CSV файл\n";
            cout << "3 - Ручной ввод параметров\n";
            cout << "Выберите действие: ";
            cin >> choice;
        }
    }

    return params;
}