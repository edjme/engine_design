#include "kinematic_output.h"
#include "path_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <limits>

using namespace std;

string KinematicOutput::generateFilename(const string &prefix)
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

void KinematicOutput::writeHeader(ofstream &file, const EngineParams &params)
{
    time_t now = time(0);

    file << "# Результаты расчета кинематики КШМ\n";
    file << "# Дата создания: " << ctime(&now);
    file << "# Параметры расчета:\n";
    file << "# Шаг угла α: " << params.step_alpha << " град.\n";
    file << "# Предел угла α: " << params.end_alpha << " град.\n";
    file << "# Радиус кривошипа " << params.radcrank << " м\n";
    file << "# λ: " << params.lyambda << "\n";
    file << "# Частота вращения: " << params.n << " об/мин\n";
    file << "# Угол развала: " << params.gamma << " град.\n";
    file << "# Угол прицепного шатуна: " << params.gammaPric << " град.\n";
    file << "# Дезаксиал: " << params.dezaxial << " м\n";
    file << "# Количество точек расчета: " << static_cast<int>(params.end_alpha / params.step_alpha) + 1 << "\n";

    // Добавляем информацию о цилиндрах если есть угол развала
    if (params.gamma != 0)
    {
        file << "# Расчет выполнен для главного и бокового цилиндров\n";
        file << "# Боковой цилиндр имеет угол развала: " << params.gamma << " град.\n";
    }
}

bool KinematicOutput::saveToCSV(const CalculationResults &results,
                                const EngineParams &params,
                                const string &filename)
{
    // Создаем директории если их нет
    if (!PathManager::ensureDirectoriesExist())
    {
        cerr << "Ошибка: не удалось создать рабочие директории" << endl;
        return false;
    }

    string outputFilename = filename.empty() ? PathManager::getOutputDirectory() + "/" + generateFilename("ksm_results") + ".csv" : filename;

    // Используем правильный разделитель путей для текущей ОС
    filesystem::path outputPath(outputFilename);
    outputFilename = outputPath.make_preferred().string();

    if (!canWriteToFile(outputFilename))
    {
        return false;
    }

#ifdef _WIN32
    // Для Windows используем широкие символы только для пути
    wstring widePath = PathManager::utf8ToWide(outputFilename);
    ofstream file(widePath.c_str(), ios::binary); // Используем ofstream и binary mode
#else
    ofstream file(outputFilename, ios::binary); // Используем binary mode
#endif

    if (!file.is_open())
    {
        cerr << "Ошибка создания CSV файла: " << outputFilename << endl;
        return false;
    }

    // ДОБАВЛЕНО: Записываем BOM для UTF-8
    file << "\xEF\xBB\xBF";

    // Записываем заголовок
    writeHeader(file, params);
    file << "\n";

    bool hasSideCylinder = !results.stroke_full_side.empty();

    // Заголовок таблицы в зависимости от наличия бокового цилиндра
    if (hasSideCylinder)
    {
        file << "alpha[град],"
             << "stroke_full_main[м];stroke1_main[м];stroke2_main[м];"
             << "velocity_full_main[м/с];velocity1_main[м/с];velocity2_main[м/с];"
             << "acceleration_full_main[м/с²];acceleration1_main[м/с²];acceleration2_main[м/с²];"
             << "betta_rod_main[рад];omega_rod_main[рад/с];eps_rod_main[рад/с²];"
             << "stroke_full_side[м];stroke1_side[м];stroke2_side[м];"
             << "velocity_full_side[м/с];velocity1_side[м/с];velocity2_side[м/с];"
             << "acceleration_full_side[м/с²];acceleration1_side[м/с²];acceleration2_side[м/с²];"
             << "betta_rod_side[рад];omega_rod_side[рад/с];eps_rod_side[рад/с²]\n";
    }
    else
    {
        file << "alpha[град];stroke_full[м];stroke1[м];stroke2[м];"
             << "velocity_full[м/с];velocity1[м/с];velocity2[м/с];"
             << "acceleration_full[м/с²];acceleration1[м/с²];acceleration2[м/с²];"
             << "betta_rod[рад];omega_rod[рад/с];eps_rod[рад/с²]\n";
    }

    // Записываем данные
    file << fixed << setprecision(6);
    size_t dataSize = results.alpha.size();
    for (size_t i = 0; i < dataSize; ++i)
    {
        file << results.alpha[i] << ";";

        if (hasSideCylinder)
        {
            // Данные для главного цилиндра
            file << results.stroke_full[i] << ";"
                 << results.stroke1[i] << ";"
                 << results.stroke2[i] << ";"
                 << results.velocity_full[i] << ";"
                 << results.velocity1[i] << ";"
                 << results.velocity2[i] << ";"
                 << results.acceleration_full[i] << ";"
                 << results.acceleration1[i] << ";"
                 << results.acceleration2[i] << ";"
                 << results.betta_rod[i] << ";"
                 << results.omega_rod[i] << ";"
                 << results.eps_rod[i] << ";";

            // Данные для бокового цилиндра
            file << results.stroke_full_side[i] << ";"
                 << results.stroke1_side[i] << ";"
                 << results.stroke2_side[i] << ";"
                 << results.velocity_full_side[i] << ";"
                 << results.velocity1_side[i] << ";"
                 << results.velocity2_side[i] << ";"
                 << results.acceleration_full_side[i] << ";"
                 << results.acceleration1_side[i] << ";"
                 << results.acceleration2_side[i] << ";"
                 << results.betta_rod_side[i] << ";"
                 << results.omega_rod_side[i] << ";"
                 << results.eps_rod_side[i];
        }
        else
        {
            // Только главный цилиндр
            file << results.stroke_full[i] << ";"
                 << results.stroke1[i] << ";"
                 << results.stroke2[i] << ";"
                 << results.velocity_full[i] << ";"
                 << results.velocity1[i] << ";"
                 << results.velocity2[i] << ";"
                 << results.acceleration_full[i] << ";"
                 << results.acceleration1[i] << ";"
                 << results.acceleration2[i] << ";"
                 << results.betta_rod[i] << ";"
                 << results.omega_rod[i] << ";"
                 << results.eps_rod[i];
        }
        file << "\n";
    }

    file.close();
    cout << "Результаты сохранены в CSV файл: " << outputFilename << endl;
    if (hasSideCylinder)
    {
        cout << "Файл содержит данные для главного и бокового цилиндров." << endl;
    }
    return true;
}

bool KinematicOutput::saveToFormattedText(const CalculationResults &results,
                                          const EngineParams &params,
                                          const string &filename)
{
    // Создаем директории если их нет
    if (!PathManager::ensureDirectoriesExist())
    {
        cerr << "Ошибка: не удалось создать рабочие директории" << endl;
        return false;
    }

    string outputFilename = filename.empty() ? PathManager::getOutputDirectory() + "/" + generateFilename("ksm_results") + ".txt" : filename;

    // Используем правильный разделитель путей для текущей ОС
    filesystem::path outputPath(outputFilename);
    outputFilename = outputPath.make_preferred().string();

    if (!canWriteToFile(outputFilename))
    {
        return false;
    }

#ifdef _WIN32
    // Для Windows используем широкие символы для поддержки Unicode путей
    wstring widePath = PathManager::utf8ToWide(outputFilename);
    ofstream file(widePath.c_str());
#else
    ofstream file(outputFilename);
#endif

    if (!file.is_open())
    {
        cerr << "Ошибка создания текстового файла: " << outputFilename << endl;
        return false;
    }

    // Записываем заголовок
    writeHeader(file, params);
    file << "\n";

    bool hasSideCylinder = !results.stroke_full_side.empty();

    // Таблица для главного цилиндра
    file << "ГЛАВНЫЙ ЦИЛИНДР:\n";
    file << "==================================================================================================================================\n";
    file << "|  α [град] |   S полн [м]  |   S1 [м]   |   S2 [м]   |  V полн [м/с] |   V1 [м/с]  |   V2 [м/с]  | A полн [м/с²] |  A1 [м/с²]  |  A2 [м/с²]  |\n";
    file << "==================================================================================================================================\n";

    // Записываем данные с форматированием
    file << fixed << setprecision(4);
    size_t dataSize = results.alpha.size();

    for (size_t i = 0; i < dataSize; ++i)
    {
        file << "| " << setw(9) << results.alpha[i] << " | "
             << setw(12) << results.stroke_full[i] << " | "
             << setw(10) << results.stroke1[i] << " | "
             << setw(10) << results.stroke2[i] << " | "
             << setw(13) << results.velocity_full[i] << " | "
             << setw(11) << results.velocity1[i] << " | "
             << setw(11) << results.velocity2[i] << " | "
             << setw(13) << results.acceleration_full[i] << " | "
             << setw(11) << results.acceleration1[i] << " | "
             << setw(11) << results.acceleration2[i] << " |\n";
    }
    file << "==================================================================================================================================\n";

    file << "УГЛОВОЕ ПЕРЕМЕЩЕНИЕ, СКОРОСТЬ, УСКОРЕНИЕ:\n";
    file << "==========================================================================\n";
    file << "|  α [град] |   B [рад]  |   w_rod [рад/с]   |   eps_rod [рад/с²]   |\n";
    file << "==========================================================================\n";

    for (size_t i = 0; i < dataSize; ++i)
    {
        file << "| " << setw(9) << results.alpha[i] << " | "
             << setw(12) << results.betta_rod[i] << " | "
             << setw(10) << results.omega_rod[i] << " | "
             << setw(11) << results.eps_rod[i] << " |\n";
    }
    file << "==================================================================================================================================\n";

    // Таблица для бокового цилиндра (если есть)
    if (hasSideCylinder)
    {
        file << "\n\nБОКОВОЙ ЦИЛИНДР (угол развала " << params.gamma << " град.):\n";
        file << "==================================================================================================================================\n";
        file << "|  α [град] |   S полн [м]  |   S1 [м]   |   S2 [м]   |  V полн [м/с] |   V1 [м/с]  |   V2 [м/с]  | A полн [м/с²] |  A1 [м/с²]  |  A2 [м/с²]  |\n";
        file << "==================================================================================================================================\n";

        for (size_t i = 0; i < dataSize; ++i)
        {
            file << "| " << setw(9) << results.alpha[i] << " | "
                 << setw(12) << results.stroke_full_side[i] << " | "
                 << setw(10) << results.stroke1_side[i] << " | "
                 << setw(10) << results.stroke2_side[i] << " | "
                 << setw(13) << results.velocity_full_side[i] << " | "
                 << setw(11) << results.velocity1_side[i] << " | "
                 << setw(11) << results.velocity2_side[i] << " | "
                 << setw(13) << results.acceleration_full_side[i] << " | "
                 << setw(11) << results.acceleration1_side[i] << " | "
                 << setw(11) << results.acceleration2_side[i] << " |\n";
        }
        file << "==================================================================================================================================\n";

        file << "УГЛОВОЕ ПЕРЕМЕЩЕНИЕ, СКОРОСТЬ, УСКОРЕНИЕ:\n";
        file << "==========================================================================\n";
        file << "|  α [град] |   B_side [рад]  |   w_rod_side [рад/с]   |   eps_rod_side [рад/с²]   |\n";
        file << "==========================================================================\n";

        // Записываем данные с форматированием
        file << fixed << setprecision(4);
        size_t dataSize = results.alpha.size();

        for (size_t i = 0; i < dataSize; ++i)
        {
            file << "| " << setw(9) << results.alpha[i] << " | "
                 << setw(12) << results.betta_rod_side[i] << " | "
                 << setw(10) << results.omega_rod_side[i] << " | "
                 << setw(11) << results.eps_rod_side[i] << " |\n";
        }
        file << "==================================================================================================================================\n";
    }

    file.close();
    cout << "Результаты сохранены в текстовый файл: " << outputFilename << endl;
    if (hasSideCylinder)
    {
        cout << "Файл содержит данные для главного и бокового цилиндров." << endl;
    }
    return true;
}

bool KinematicOutput::saveSummary(const CalculationResults &results,
                                  const EngineParams &params,
                                  const string &filename)
{
    // Создаем директории если их нет
    if (!PathManager::ensureDirectoriesExist())
    {
        cerr << "Ошибка: не удалось создать рабочие директории" << endl;
        return false;
    }

    string outputFilename = filename.empty() ? PathManager::getOutputDirectory() + "/" + generateFilename("ksm_summary") + ".csv" : filename;

    // Используем правильный разделитель путей для текущей ОС
    filesystem::path outputPath(outputFilename);
    outputFilename = outputPath.make_preferred().string();

    if (!canWriteToFile(outputFilename))
    {
        return false;
    }

#ifdef _WIN32
    // Для Windows используем широкие символы только для пути
    wstring widePath = PathManager::utf8ToWide(outputFilename);
    ofstream file(widePath.c_str(), ios::binary); // Используем ofstream и binary mode
#else
    ofstream file(outputFilename, ios::binary); // Используем binary mode
#endif

    if (!file.is_open())
    {
        cerr << "Ошибка создания файла: " << outputFilename << endl;
        return false;
    }

    // ДОБАВЛЕНО: Записываем BOM для UTF-8
    file << "\xEF\xBB\xBF";

    writeHeader(file, params);
    file << "\n";

    bool hasSideCylinder = !results.stroke_full_side.empty();

    // Заголовок для основных параметров
    if (hasSideCylinder)
    {
        file << "alpha[град];stroke_full_main[м];velocity_full_main[м/с];acceleration_full_main[м/с²];"
             << "betta_rod_main[рад];omega_rod_main[рад/с];eps_rod_main[рад/с²];"
             << "stroke_full_side[м];velocity_full_side[м/с];acceleration_full_side[м/с²];"
             << "betta_rod_side[рад];omega_rod_side[рад/с];eps_rod_side[рад/с²]\n";
    }
    else
    {
        file << "alpha[град];stroke_full[м];velocity_full[м/с];acceleration_full[м/с²];"
             << "betta_rod[рад];omega_rod[рад/с];eps_rod[рад/с²]\n";
    }

    // Записываем только основные данные
    file << fixed << setprecision(6);
    size_t dataSize = results.alpha.size();
    for (size_t i = 0; i < dataSize; ++i)
    {
        file << results.alpha[i] << ";"
             << results.stroke_full[i] << ";"
             << results.velocity_full[i] << ";"
             << results.acceleration_full[i] << ";"
             << results.betta_rod[i] << ";"
             << results.omega_rod[i] << ";"
             << results.eps_rod[i];

        if (hasSideCylinder)
        {
            file << ";" << results.stroke_full_side[i] << ";"
                 << results.velocity_full_side[i] << ";"
                 << results.acceleration_full_side[i] << ";"
                 << results.betta_rod_side[i] << ";"
                 << results.omega_rod_side[i] << ";"
                 << results.eps_rod_side[i];
        }
        file << "\n";
    }

    file.close();
    cout << "Сводные результаты сохранены в файл: " << outputFilename << endl;
    if (hasSideCylinder)
    {
        cout << "Файл содержит данные для главного и бокового цилиндров." << endl;
    }
    return true;
}

void KinematicOutput::exportWithMenu(const CalculationResults &results,
                                     const EngineParams &params)
{
    int choice;
    string filename;

    cout << "\n=== ЭКСПОРТ РЕЗУЛЬТАТОВ ===\n";
    cout << "1 - Сохранить в CSV (полные данные)\n";
    cout << "2 - Сохранить в текстовый файл (форматированный)\n";
    cout << "3 - Сохранить сводку (только основные параметры)\n";
    cout << "4 - Сохранить во все форматы\n";
    cout << "0 - Не сохранять\n";
    cout << "Выберите действие: ";
    cin >> choice;

    if (choice == 0)
        return;

    if (choice == 1 || choice == 4)
    {
        cout << "Использовать автоматическое имя для CSV? (1 - ДА / 2 - НЕТ): ";
        char answer;
        cin >> answer;
        if (answer == '2')
        {
            cout << "Введите имя CSV файла: ";
            cin >> filename;
            // Добавляем путь к папке если пользователь ввел только имя файла
            if (filename.find('/') == string::npos && filename.find('\\') == string::npos)
            {
                filename = PathManager::getOutputDirectory() + "/" + filename;
            }
        }
        saveToCSV(results, params, filename);
    }

    if (choice == 2 || choice == 4)
    {
        filename.clear();
        cout << "Использовать автоматическое имя для текстового файла? (1 - ДА / 2 - НЕТ): ";
        char answer;
        cin >> answer;
        if (answer == '2')
        {
            cout << "Введите имя текстового файла: ";
            cin >> filename;
            // Добавляем путь к папке если пользователь ввел только имя файла
            if (filename.find('/') == string::npos && filename.find('\\') == string::npos)
            {
                filename = PathManager::getOutputDirectory() + "/" + filename;
            }
        }
        saveToFormattedText(results, params, filename);
    }

    if (choice == 3 || choice == 4)
    {
        filename.clear();
        cout << "Использовать автоматическое имя для сводки? (1 - ДА / 2 - НЕТ): ";
        char answer;
        cin >> answer;
        if (answer == '2')
        {
            cout << "Введите имя файла сводки: ";
            cin >> filename;
            // Добавляем путь к папке если пользователь ввел только имя файла
            if (filename.find('/') == string::npos && filename.find('\\') == string::npos)
            {
                filename = PathManager::getOutputDirectory() + "/" + filename;
            }
        }
        saveSummary(results, params, filename);
    }
}

bool KinematicOutput::canWriteToFile(const string &filename)
{
#ifdef _WIN32
    // Для Windows используем широкие символы
    wstring widePath = PathManager::utf8ToWide(filename);
    ofstream testFile(widePath.c_str(), ios::app);
#else
    ofstream testFile(filename, ios::app);
#endif

    if (!testFile.is_open())
    {
        cerr << "Ошибка: Невозможно записать в файл " << filename << endl;
        cerr << "Проверьте права доступа или закройте файл если он открыт в другой программе." << endl;
        return false;
    }
    testFile.close();

    // Удаляем тестовый файл
#ifdef _WIN32
    _wremove(widePath.c_str());
#else
    remove(filename.c_str());
#endif
    return true;
}