#include "ind_diag_input.h"
#include "ind_path_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

using namespace std;

// =======================
// Проверка и создание файла по умолчанию
// =======================
void IndInput::initializeDefaultConfig()
{
    std::string defaultFile = IndPathManager::getInputDirectory() + "/ind_input.csv";

#ifdef _WIN32
    std::wstring wide_filename = IndPathManager::utf8ToWide(defaultFile);
    std::ifstream testFile(wide_filename.c_str());
#else
    std::ifstream testFile(defaultFile);
#endif

    if (!testFile.is_open())
    {
        std::cout << "Файл конфигурации по умолчанию не найден. Создаю..." << std::endl;
        IndcreateDefaultCSV();
    }
    else
    {
        testFile.close();
        std::cout << "Файл конфигурации по умолчанию найден." << std::endl;
    }
}

// =======================
// Проверка корректности параметров
// =======================
bool IndInput::IndvalidateParams(const IndParams &params)
{
    if (params.step_alpha <= 0 || params.step_alpha > 360)
    {
        std::cerr << "Ошибка: шаг угла должен быть в диапазоне (0, 360].\n";
        return false;
    }
    if (params.end_alpha <= 0 || params.end_alpha > 720)
    {
        std::cerr << "Ошибка: предельный угол должен быть в диапазоне (0, 720].\n";
        return false;
    }
    if (params.radcrank <= 0)
    {
        std::cerr << "Ошибка: радиус кривошипа должен быть положительным.\n";
        return false;
    }
    if (params.lyambda <= 0 || params.lyambda >= 1)
    {
        std::cerr << "Ошибка: λ должна быть в диапазоне (0, 1).\n";
        return false;
    }
    if (params.epsilent <= 1)
    {
        std::cerr << "Ошибка: степень сжатия ε должна быть > 1.\n";
        return false;
    }
    if (params.diam_cyl <= 0)
    {
        std::cerr << "Ошибка: диаметр цилиндра должен быть положительным.\n";
        return false;
    }
    if (params.n <= 0)
    {
        std::cerr << "Ошибка: частота вращения должна быть положительной.\n";
        return false;
    }
    if (params.tau != 2 && params.tau != 4)
    {
        std::cerr << "Ошибка: тактность может быть только 2 или 4.\n";
        return false;
    }
    return true;
}

// =======================
// Генерация имени файла с датой и временем
// =======================
std::string IndInput::IndgenerateFilename(const std::string &prefix)
{
    time_t now = time(0);
    tm *localTime = localtime(&now);

    std::stringstream filename;
    filename << prefix << "_"
             << std::setw(2) << std::setfill('0') << localTime->tm_hour << "-"
             << std::setw(2) << std::setfill('0') << localTime->tm_min << "-"
             << std::setw(2) << std::setfill('0') << localTime->tm_sec << "_"
             << std::setw(2) << std::setfill('0') << localTime->tm_mday << "."
             << std::setw(2) << std::setfill('0') << (localTime->tm_mon + 1) << "."
             << (1900 + localTime->tm_year);

    return filename.str();
}

// =======================
// Создание CSV по умолчанию
// =======================
void IndInput::IndcreateDefaultCSV(const std::string &filename)
{
    if (!IndPathManager::ensureDirectoriesExist())
    {
        std::cerr << "Ошибка: не удалось создать рабочие директории.\n";
        return;
    }

    std::string fullFilename = filename.empty()
                                   ? IndPathManager::getInputDirectory() + "/ind_input.csv"
                                   : filename;

#ifdef _WIN32
    std::wstring wide_filename = IndPathManager::utf8ToWide(fullFilename);
    std::ofstream file(wide_filename.c_str(), std::ios::binary);
#else
    std::ofstream file(fullFilename, std::ios::binary);
#endif

    if (!file.is_open())
    {
        std::cerr << "Ошибка создания CSV файла: " << fullFilename << std::endl;
        return;
    }

    file << "\xEF\xBB\xBF";
    file << "параметр;значение;ед.;описание\n";
    file << "step_alpha;5;град.;шаг угла\n";
    file << "end_alpha;360;град.;предельный угол\n";
    file << "radcrank;0.02;м;радиус кривошипа\n";
    file << "lyambda;0.3;-;геометрическая характеристика КШМ λ\n";
    file << "diam_cyl;0.12;м;диаметр цилиндра\n";
    file << "n;4800;об/мин;частота вращения коленвала\n";
    file << "epsilent;17;-;степень сжатия ε\n";
    file << "p_a;90000;Па;давление на впуске\n";
    file << "p_r;110000;Па;давление на выпуске\n";
    file << "n_1;1.38;-;показатель политропы сжатия\n";
    file << "n_2;1.22;-;показатель политропы расширения\n";
    file << "lymbda_z;2.0;-;степень повышения давления (Pz/Pc)\n";
    file << "ro;1.4;-;степень предварительного расширения (Vz'/Vz)\n";
    file << "tau;4;-;тактность (2 или 4)\n";

    file.close();
    std::cout << "Создан CSV файл конфигурации по умолчанию: " << fullFilename << std::endl;
}

// =======================
// Чтение CSV
// =======================
bool IndInput::IndreadCSVConfig(const std::string &filename, IndParams &params)
{
    std::string fullFilename = filename;
    if (filename.find('/') == std::string::npos && filename.find('\\') == std::string::npos)
        fullFilename = IndPathManager::getInputDirectory() + "/" + filename;

    std::ifstream file(fullFilename);
    if (!file.is_open())
    {
        std::cerr << "Ошибка открытия файла: " << fullFilename << std::endl;
        return false;
    }

    std::string line;
    int lineNum = 0;
    while (getline(file, line))
    {
        lineNum++;
        if (lineNum == 1 || line.empty())
            continue;

        std::stringstream ss(line);
        std::string param, value;
        getline(ss, param, ';');
        getline(ss, value, ';');

        double v = atof(value.c_str());

        if (param == "step_alpha")
            params.step_alpha = v;
        else if (param == "end_alpha")
            params.end_alpha = v;
        else if (param == "radcrank")
            params.radcrank = v;
        else if (param == "lyambda")
            params.lyambda = v;
        else if (param == "diam_cyl")
            params.diam_cyl = v;
        else if (param == "n")
            params.n = v;
        else if (param == "epsilent")
            params.epsilent = v;
        else if (param == "p_a")
            params.p_a = v;
        else if (param == "p_r")
            params.p_r = v;
        else if (param == "n_1")
            params.n_1 = v;
        else if (param == "n_2")
            params.n_2 = v;
        else if (param == "lymbda_z")
            params.lymbda_z = v;
        else if (param == "ro")
            params.ro = v;
        else if (param == "tau")
            params.tau = v;
    }
    file.close();
    return true;
}

// =======================
// Вывод параметров на экран
// =======================
void IndInput::IndprintParams(const IndParams &p)
{
    std::cout << "\n=== ЗАГРУЖЕННЫЕ ПАРАМЕТРЫ ===\n";
    std::cout << "Шаг α: " << p.step_alpha << "°\n";
    std::cout << "Предел α: " << p.end_alpha << "°\n";
    std::cout << "Радиус кривошипа: " << p.radcrank << " м\n";
    std::cout << "λ: " << p.lyambda << "\n";
    std::cout << "Диаметр цилиндра: " << p.diam_cyl << " м\n";
    std::cout << "Частота вращения: " << p.n << " об/мин\n";
    std::cout << "Степень сжатия ε: " << p.epsilent << "\n";
    std::cout << "Pвпуска: " << p.p_a << " Па\n";
    std::cout << "Pвыпуска: " << p.p_r << " Па\n";
    std::cout << "n₁ (сжатие): " << p.n_1 << "\n";
    std::cout << "n₂ (расширение): " << p.n_2 << "\n";
    std::cout << "λz (Pz/Pc): " << p.lymbda_z << "\n";
    std::cout << "ρ (Vz'/Vz): " << p.ro << "\n";
    std::cout << "Тактность: " << p.tau << "\n";
    std::cout << "=================================\n";
}

// =======================
// Меню загрузки параметров
// =======================
IndParams IndInput::IndloadParamsWithMenu()
{
    initializeDefaultConfig();

    IndParams params;
    int choice;
    std::string filename;
    bool paramsValid = false;

    std::cout << "\n=== ЗАГРУЗКА ПАРАМЕТРОВ ===\n";
    std::cout << "1 - Использовать файл по умолчанию (ind_input.csv)\n";
    std::cout << "2 - Загрузить другой CSV файл\n";
    std::cout << "3 - Ручной ввод параметров\n";
    std::cout << "Выберите действие: ";
    std::cin >> choice;

    while (!paramsValid)
    {
        switch (choice)
        {
        case 1:
            if (!IndreadCSVConfig("ind_input.csv", params))
                params = IndmanualInput();
            break;
        case 2:
            std::cout << "Введите имя CSV файла: ";
            std::cin >> filename;
            if (!IndreadCSVConfig(filename, params))
                params = IndmanualInput();
            break;
        case 3:
        default:
            params = IndmanualInput();
            break;
        }
        paramsValid = IndvalidateParams(params);
        if (!paramsValid)
        {
            std::cout << "\nНекорректные данные, повторите ввод.\n";
            choice = 3;
        }
    }
    return params;
}

// =======================
// Ручной ввод параметров
// =======================
IndParams IndInput::IndmanualInput()
{
    IndParams p;
    std::cout << "\n=== РУЧНОЙ ВВОД ПАРАМЕТРОВ ===\n";
    std::cout << "Шаг α (°): ";
    std::cin >> p.step_alpha;
    std::cout << "Предел α (°): ";
    std::cin >> p.end_alpha;
    std::cout << "Радиус кривошипа (м): ";
    std::cin >> p.radcrank;
    std::cout << "λ: ";
    std::cin >> p.lyambda;
    std::cout << "Диаметр цилиндра (м): ";
    std::cin >> p.diam_cyl;
    std::cout << "Частота вращения (об/мин): ";
    std::cin >> p.n;
    std::cout << "Степень сжатия ε: ";
    std::cin >> p.epsilent;
    std::cout << "Давление на впуске (Па): ";
    std::cin >> p.p_a;
    std::cout << "Давление на выпуске (Па): ";
    std::cin >> p.p_r;
    std::cout << "n₁ (сжатие): ";
    std::cin >> p.n_1;
    std::cout << "n₂ (расширение): ";
    std::cin >> p.n_2;
    std::cout << "λz (Pz/Pc): ";
    std::cin >> p.lymbda_z;
    std::cout << "ρ (Vz'/Vz): ";
    std::cin >> p.ro;
    std::cout << "Тактность (2 или 4): ";
    std::cin >> p.tau;
    return p;
}
