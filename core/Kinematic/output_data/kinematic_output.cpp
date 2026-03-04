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
    bool hasSideCylinder = !results.cylinder_stroke_full_side.empty();

    // Проверяем количество цилиндров
    size_t numCylinders = results.cylinder_stroke_full.size();
    if (numCylinders == 0) {
        cerr << "Ошибка: нет данных о цилиндрах" << endl;
        return false;
    }

    // Определяем количество точек по первому цилиндру
    size_t dataSize = (numCylinders > 0 && !results.cylinder_stroke_full[0].empty()) 
                      ? results.cylinder_stroke_full[0].size() : 0;
    
    if (dataSize == 0) {
        cerr << "Ошибка: нет данных для сохранения" << endl;
        return false;
    }

    // Заголовок таблицы
    file << "alpha[град];";
    
    // Для каждого цилиндра добавляем колонки
    if (hasSideCylinder) {
    for (size_t cyl = 0; cyl < numCylinders / 2; ++cyl)
    {
        file << "cyl" << (cyl+1) << "_stroke_full[м];"
             << "cyl" << (cyl+1) << "_stroke1[м];"
             << "cyl" << (cyl+1) << "_stroke2[м];"
             << "cyl" << (cyl+1) << "_velocity_full[м/с];"
             << "cyl" << (cyl+1) << "_velocity1[м/с];"
             << "cyl" << (cyl+1) << "_velocity2[м/с];"
             << "cyl" << (cyl+1) << "_acceleration_full[м/с²];"
             << "cyl" << (cyl+1) << "_acceleration1[м/с²];"
             << "cyl" << (cyl+1) << "_acceleration2[м/с²];"
             << "cyl" << (cyl+1) << "_betta_rod[рад];"
             << "cyl" << (cyl+1) << "_omega_rod[рад/с];"
             << "cyl" << (cyl+1) << "_eps_rod[рад/с²]";
        
        // Если есть боковой цилиндр для этого основного
        if (hasSideCylinder && cyl < results.cylinder_stroke_full_side.size()) {
            file << ";cyl" << (cyl+1) << "_side_stroke_full[м];"
                 << "cyl" << (cyl+1) << "_side_stroke1[м];"
                 << "cyl" << (cyl+1) << "_side_stroke2[м];"
                 << "cyl" << (cyl+1) << "_side_velocity_full[м/с];"
                 << "cyl" << (cyl+1) << "_side_velocity1[м/с];"
                 << "cyl" << (cyl+1) << "_side_velocity2[м/с];"
                 << "cyl" << (cyl+1) << "_side_acceleration_full[м/с²];"
                 << "cyl" << (cyl+1) << "_side_acceleration1[м/с²];"
                 << "cyl" << (cyl+1) << "_side_acceleration2[м/с²];"
                 << "cyl" << (cyl+1) << "_side_betta_rod[рад];"
                 << "cyl" << (cyl+1) << "_side_omega_rod[рад/с];"
                 << "cyl" << (cyl+1) << "_side_eps_rod[рад/с²]";
        }
        
        // Если не последний цилиндр, добавляем разделитель
        if (cyl != numCylinders - 1) {
            file << ";";
        }
    }
}
else {
for (size_t cyl = 0; cyl < numCylinders ; ++cyl)
    {
        file << "cyl" << (cyl+1) << "_stroke_full[м];"
             << "cyl" << (cyl+1) << "_stroke1[м];"
             << "cyl" << (cyl+1) << "_stroke2[м];"
             << "cyl" << (cyl+1) << "_velocity_full[м/с];"
             << "cyl" << (cyl+1) << "_velocity1[м/с];"
             << "cyl" << (cyl+1) << "_velocity2[м/с];"
             << "cyl" << (cyl+1) << "_acceleration_full[м/с²];"
             << "cyl" << (cyl+1) << "_acceleration1[м/с²];"
             << "cyl" << (cyl+1) << "_acceleration2[м/с²];"
             << "cyl" << (cyl+1) << "_betta_rod[рад];"
             << "cyl" << (cyl+1) << "_omega_rod[рад/с];"
             << "cyl" << (cyl+1) << "_eps_rod[рад/с²]";
        
                
        // Если не последний цилиндр, добавляем разделитель
        if (cyl != numCylinders - 1) {
            file << ";";
        }
    }

};
    file << "\n";

    // Записываем данные
    file << fixed << setprecision(6);
    
    for (size_t i = 0; i < dataSize; ++i)
    {
        // Угол поворота (alpha одинаков для всех цилиндров)
        if (i < results.alpha.size()) {
            file << results.alpha[i] << ";";
        } else {
            file << "0.0;";  // Значение по умолчанию если нет alpha
        }

        // Данные для каждого цилиндра
        if (hasSideCylinder) {
        for (size_t cyl = 0; cyl < numCylinders / 2; ++cyl)
        {
            // Проверяем, что вектор для этого цилиндра существует
            bool cylHasData = (cyl < results.cylinder_stroke_full.size() && 
                              i < results.cylinder_stroke_full[cyl].size());
            
            // Главный цилиндр
            if (cylHasData) {
                file << results.cylinder_stroke_full[cyl][i] << ";"
                     << results.cylinder_stroke1[cyl][i] << ";"
                     << results.cylinder_stroke2[cyl][i] << ";"
                     << results.cylinder_velocity_full[cyl][i] << ";"
                     << results.cylinder_velocity1[cyl][i] << ";"
                     << results.cylinder_velocity2[cyl][i] << ";"
                     << results.cylinder_acceleration_full[cyl][i] << ";"
                     << results.cylinder_acceleration1[cyl][i] << ";"
                     << results.cylinder_acceleration2[cyl][i] << ";"
                     << results.cylinder_betta_rod[cyl][i] << ";"
                     << results.cylinder_omega_rod[cyl][i] << ";"
                     << results.cylinder_eps_rod[cyl][i];
            } else {
                // Заполняем нулями если данных нет
                file << "0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0";
            }

            // Боковой цилиндр (если есть)
            if (hasSideCylinder && cyl < results.cylinder_stroke_full_side.size()) {
                bool sideHasData = (i < results.cylinder_stroke_full_side[cyl].size());
                
                file << ";";  // Разделитель между главным и боковым
                
                if (sideHasData) {
                    file << results.cylinder_stroke_full_side[cyl][i] << ";"
                         << results.cylinder_stroke1_side[cyl][i] << ";"
                         << results.cylinder_stroke2_side[cyl][i] << ";"
                         << results.cylinder_velocity_full_side[cyl][i] << ";"
                         << results.cylinder_velocity1_side[cyl][i] << ";"
                         << results.cylinder_velocity2_side[cyl][i] << ";"
                         << results.cylinder_acceleration_full_side[cyl][i] << ";"
                         << results.cylinder_acceleration1_side[cyl][i] << ";"
                         << results.cylinder_acceleration2_side[cyl][i] << ";"
                         << results.cylinder_betta_rod_side[cyl][i] << ";"
                         << results.cylinder_omega_rod_side[cyl][i] << ";"
                         << results.cylinder_eps_rod_side[cyl][i];
                } else {
                    file << "0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0";
                }
            }
            
            // Разделитель между цилиндрами (кроме последнего)
            if (cyl != numCylinders - 1) {
                file << ";";
            }
        }
    }//////////////////
     else {
for (size_t cyl = 0; cyl < numCylinders; ++cyl)
        {
            // Проверяем, что вектор для этого цилиндра существует
            bool cylHasData = (cyl < results.cylinder_stroke_full.size() && 
                              i < results.cylinder_stroke_full[cyl].size());
            
            // Главный цилиндр
            if (cylHasData) {
                file << results.cylinder_stroke_full[cyl][i] << ";"
                     << results.cylinder_stroke1[cyl][i] << ";"
                     << results.cylinder_stroke2[cyl][i] << ";"
                     << results.cylinder_velocity_full[cyl][i] << ";"
                     << results.cylinder_velocity1[cyl][i] << ";"
                     << results.cylinder_velocity2[cyl][i] << ";"
                     << results.cylinder_acceleration_full[cyl][i] << ";"
                     << results.cylinder_acceleration1[cyl][i] << ";"
                     << results.cylinder_acceleration2[cyl][i] << ";"
                     << results.cylinder_betta_rod[cyl][i] << ";"
                     << results.cylinder_omega_rod[cyl][i] << ";"
                     << results.cylinder_eps_rod[cyl][i];
            } else {
                // Заполняем нулями если данных нет
                file << "0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0";
            }

            
            
            // Разделитель между цилиндрами (кроме последнего)
            if (cyl != numCylinders - 1) {
                file << ";";
            }
        }
     };  
        file << "\n";
    }

    file.close();
    cout << "Результаты сохранены в CSV файл: " << outputFilename << endl;
    cout << "Сохранено " << numCylinders << " цилиндров, " 
         << dataSize << " точек расчета" << endl;
    
    if (hasSideCylinder) {
        cout << "Включая данные боковых цилиндров" << endl;
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

    string outputFilename = filename.empty() 
        ? PathManager::getOutputDirectory() + "/" + generateFilename("ksm_results") + ".txt" 
        : filename;

    filesystem::path outputPath(outputFilename);
    outputFilename = outputPath.make_preferred().string();

    if (!canWriteToFile(outputFilename))
    {
        return false;
    }

#ifdef _WIN32
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

    // Записываем заголовок с параметрами
    writeHeader(file, params);
    file << "\n\n";

    // Определяем количество цилиндров
    size_t numCylinders = results.cylinder_stroke_full.size();
    if (numCylinders == 0) {
        cerr << "Ошибка: нет данных о цилиндрах" << endl;
        return false;
    }

    // Определяем количество точек по первому цилиндру
    size_t dataSize = (numCylinders > 0 && !results.cylinder_stroke_full[0].empty()) 
                      ? results.cylinder_stroke_full[0].size() : 0;
    
    if (dataSize == 0) {
        cerr << "Ошибка: нет данных для сохранения" << endl;
        return false;
    }

    // Проверяем наличие боковых цилиндров
    bool hasSideCylinder = !results.cylinder_stroke_full_side.empty();

    // Для каждого цилиндра выводим отдельную таблицу
    if (hasSideCylinder) {
    for (size_t cyl = 0; cyl < numCylinders / 2; ++cyl)
    {
        file << "ЦИЛИНДР " << (cyl + 1) << ":\n";
        file << string(140, '=') << "\n";
        
        // Таблица перемещений, скоростей и ускорений
        file << "| α [град] |   S полн [м]  |   S1 [м]   |   S2 [м]   |  V полн [м/с] |   V1 [м/с]  |   V2 [м/с]  | A полн [м/с²] |  A1 [м/с²]  |  A2 [м/с²]  |\n";
        file << string(140, '=') << "\n";

        file << fixed << setprecision(4);
        
        // Выводим все точки для этого цилиндра
        for (size_t i = 0; i < dataSize; ++i)
        {
            // Проверяем наличие данных
            bool hasData = (cyl < results.cylinder_stroke_full.size() && 
                           i < results.cylinder_stroke_full[cyl].size());
            
            if (!hasData) continue;
            
            // Угол поворота (берем из общего массива alpha или используем индекс)
            double alpha = 0.0;
            if (i < results.alpha.size()) {
                alpha = results.alpha[i];
            } else {
                // Если нет общего alpha, вычисляем по шагу
                alpha = i * params.step_alpha;
            }
            
            file << "| " << setw(7) << alpha << " | "
                 << setw(12) << results.cylinder_stroke_full[cyl][i] << " | "
                 << setw(10) << results.cylinder_stroke1[cyl][i] << " | "
                 << setw(10) << results.cylinder_stroke2[cyl][i] << " | "
                 << setw(13) << results.cylinder_velocity_full[cyl][i] << " | "
                 << setw(11) << results.cylinder_velocity1[cyl][i] << " | "
                 << setw(11) << results.cylinder_velocity2[cyl][i] << " | "
                 << setw(13) << results.cylinder_acceleration_full[cyl][i] << " | "
                 << setw(11) << results.cylinder_acceleration1[cyl][i] << " | "
                 << setw(11) << results.cylinder_acceleration2[cyl][i] << " |\n";
        }
        file << string(140, '=') << "\n\n";

        // Таблица угловых параметров шатуна
        file << "УГЛОВОЕ ПЕРЕМЕЩЕНИЕ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА:\n";
        file << string(80, '=') << "\n";
        file << "| α [град] |   β [рад]  |   ω шатуна [рад/с]   |   ε шатуна [рад/с²]   |\n";
        file << string(80, '=') << "\n";

        for (size_t i = 0; i < dataSize; ++i)
        {
            bool hasRodData = (cyl < results.cylinder_betta_rod.size() && 
                              i < results.cylinder_betta_rod[cyl].size());
            
            if (!hasRodData) continue;
            
            double alpha = 0.0;
            if (i < results.alpha.size()) {
                alpha = results.alpha[i];
            } else {
                alpha = i * params.step_alpha;
            }
            
            file << "| " << setw(7) << alpha << " | "
                 << setw(10) << results.cylinder_betta_rod[cyl][i] << " | "
                 << setw(20) << results.cylinder_omega_rod[cyl][i] << " | "
                 << setw(20) << results.cylinder_eps_rod[cyl][i] << " |\n";
        }
        file << string(80, '=') << "\n\n";

        // Если есть боковой цилиндр для этого основного
        if (hasSideCylinder && cyl < results.cylinder_stroke_full_side.size())
        {
            file << "БОКОВОЙ ЦИЛИНДР " << (cyl + 1) << " (угол развала " << params.gamma << "°):\n";
            file << string(120, '=') << "\n";
            file << "| α [град] |   S полн [м]  |   S1 [м]   |   S2 [м]   |  V полн [м/с] |   V1 [м/с]  |   V2 [м/с]  | A полн [м/с²] |  A1 [м/с²]  |  A2 [м/с²]  |\n";
            file << string(120, '=') << "\n";

            for (size_t i = 0; i < dataSize; ++i)
            {
                bool hasSideData = (i < results.cylinder_stroke_full_side[cyl].size());
                
                if (!hasSideData) continue;
                
                double alpha = 0.0;
                if (i < results.alpha.size()) {
                    alpha = results.alpha[i];
                } else {
                    alpha = i * params.step_alpha;
                }
                
                file << "| " << setw(7) << alpha << " | "
                     << setw(12) << results.cylinder_stroke_full_side[cyl][i] << " | "
                     << setw(10) << results.cylinder_stroke1_side[cyl][i] << " | "
                     << setw(10) << results.cylinder_stroke2_side[cyl][i] << " | "
                     << setw(13) << results.cylinder_velocity_full_side[cyl][i] << " | "
                     << setw(11) << results.cylinder_velocity1_side[cyl][i] << " | "
                     << setw(11) << results.cylinder_velocity2_side[cyl][i] << " | "
                     << setw(13) << results.cylinder_acceleration_full_side[cyl][i] << " | "
                     << setw(11) << results.cylinder_acceleration1_side[cyl][i] << " | "
                     << setw(11) << results.cylinder_acceleration2_side[cyl][i] << " |\n";
            }
            file << string(120, '=') << "\n\n";

            // Угловые параметры бокового шатуна
            file << "УГЛОВОЕ ПЕРЕМЕЩЕНИЕ, СКОРОСТЬ И УСКОРЕНИЕ БОКОВОГО ШАТУНА:\n";
            file << string(80, '=') << "\n";
            file << "| α [град] |   β бок [рад]  |   ω бок [рад/с]   |   ε бок [рад/с²]   |\n";
            file << string(80, '=') << "\n";

            for (size_t i = 0; i < dataSize; ++i)
            {
                bool hasSideRodData = (cyl < results.cylinder_betta_rod_side.size() && 
                                      i < results.cylinder_betta_rod_side[cyl].size());
                
                if (!hasSideRodData) continue;
                
                double alpha = 0.0;
                if (i < results.alpha.size()) {
                    alpha = results.alpha[i];
                } else {
                    alpha = i * params.step_alpha;
                }
                
                file << "| " << setw(7) << alpha << " | "
                     << setw(13) << results.cylinder_betta_rod_side[cyl][i] << " | "
                     << setw(17) << results.cylinder_omega_rod_side[cyl][i] << " | "
                     << setw(18) << results.cylinder_eps_rod_side[cyl][i] << " |\n";
            }
            file << string(80, '=') << "\n\n";
        }

        // Разделитель между цилиндрами (если не последний)
        if (cyl != numCylinders - 1) {
            file << "\n" << string(80, '*') << "\n\n";
        }
    }
}
else {
    for (size_t cyl = 0; cyl < numCylinders; ++cyl)
    {
        file << "ЦИЛИНДР " << (cyl + 1) << ":\n";
        file << string(140, '=') << "\n";
        
        // Таблица перемещений, скоростей и ускорений
        file << "| α [град] |   S полн [м]  |   S1 [м]   |   S2 [м]   |  V полн [м/с] |   V1 [м/с]  |   V2 [м/с]  | A полн [м/с²] |  A1 [м/с²]  |  A2 [м/с²]  |\n";
        file << string(140, '=') << "\n";

        file << fixed << setprecision(4);
        
        // Выводим все точки для этого цилиндра
        for (size_t i = 0; i < dataSize; ++i)
        {
            // Проверяем наличие данных
            bool hasData = (cyl < results.cylinder_stroke_full.size() && 
                           i < results.cylinder_stroke_full[cyl].size());
            
            if (!hasData) continue;
            
            // Угол поворота (берем из общего массива alpha или используем индекс)
            double alpha = 0.0;
            if (i < results.alpha.size()) {
                alpha = results.alpha[i];
            } else {
                // Если нет общего alpha, вычисляем по шагу
                alpha = i * params.step_alpha;
            }
            
            file << "| " << setw(7) << alpha << " | "
                 << setw(12) << results.cylinder_stroke_full[cyl][i] << " | "
                 << setw(10) << results.cylinder_stroke1[cyl][i] << " | "
                 << setw(10) << results.cylinder_stroke2[cyl][i] << " | "
                 << setw(13) << results.cylinder_velocity_full[cyl][i] << " | "
                 << setw(11) << results.cylinder_velocity1[cyl][i] << " | "
                 << setw(11) << results.cylinder_velocity2[cyl][i] << " | "
                 << setw(13) << results.cylinder_acceleration_full[cyl][i] << " | "
                 << setw(11) << results.cylinder_acceleration1[cyl][i] << " | "
                 << setw(11) << results.cylinder_acceleration2[cyl][i] << " |\n";
        }
        file << string(140, '=') << "\n\n";

        // Таблица угловых параметров шатуна
        file << "УГЛОВОЕ ПЕРЕМЕЩЕНИЕ, СКОРОСТЬ И УСКОРЕНИЕ ШАТУНА:\n";
        file << string(80, '=') << "\n";
        file << "| α [град] |   β [рад]  |   ω шатуна [рад/с]   |   ε шатуна [рад/с²]   |\n";
        file << string(80, '=') << "\n";

        for (size_t i = 0; i < dataSize; ++i)
        {
            bool hasRodData = (cyl < results.cylinder_betta_rod.size() && 
                              i < results.cylinder_betta_rod[cyl].size());
            
            if (!hasRodData) continue;
            
            double alpha = 0.0;
            if (i < results.alpha.size()) {
                alpha = results.alpha[i];
            } else {
                alpha = i * params.step_alpha;
            }
            
            file << "| " << setw(7) << alpha << " | "
                 << setw(10) << results.cylinder_betta_rod[cyl][i] << " | "
                 << setw(20) << results.cylinder_omega_rod[cyl][i] << " | "
                 << setw(20) << results.cylinder_eps_rod[cyl][i] << " |\n";
        }
        file << string(80, '=') << "\n\n";
    }
}

    // Сводная информация в конце файла
    file << "\n" << string(60, '=') << "\n";
    file << "СВОДНАЯ ИНФОРМАЦИЯ:\n";
    file << string(60, '=') << "\n";
    file << "• Количество точек расчета: " << dataSize << "\n";
    file << "• Шаг угла α: " << params.step_alpha << "°\n";
    file << "• Предел угла α: " << params.end_alpha << "°\n";
    file << "• Радиус кривошипа: " << params.radcrank << " м\n";
    file << "• Частота вращения: " << params.n << " об/мин\n";
    
    if (params.gamma != 0.0) {
        file << "• Угол развала: " << params.gamma << "°\n";
    }
    if (params.dezaxial != 0.0) {
        file << "• Дезаксиал: " << params.dezaxial << " м\n";
    }
    
    

    file.close();
    
    cout << "Результаты сохранены в текстовый файл: " << outputFilename << endl;
    cout << "Сохранено " << numCylinders << " цилиндров" << endl;
    if (hasSideCylinder) {
        cout << "Включая данные боковых цилиндров" << endl;
    }
    
    return true;
}

bool KinematicOutput::saveSummary(const CalculationResults &results,
                                  const EngineParams &params,
                                  const string &filename)
{
    if (!PathManager::ensureDirectoriesExist()) {
        cerr << "Ошибка: не удалось создать рабочие директории" << endl;
        return false;
    }

    string outputFilename = filename.empty() ? PathManager::getOutputDirectory() + "/" + generateFilename("ksm_summary") + ".csv" : filename;

    filesystem::path outputPath(outputFilename);
    outputFilename = outputPath.make_preferred().string();

    if (!canWriteToFile(outputFilename))
        return false;

#ifdef _WIN32
    wstring widePath = PathManager::utf8ToWide(outputFilename);
    ofstream file(widePath.c_str(), ios::binary);
#else
    ofstream file(outputFilename, ios::binary);
#endif

    if (!file.is_open()) {
        cerr << "Ошибка создания файла: " << outputFilename << endl;
        return false;
    }

    file << "\xEF\xBB\xBF"; // UTF-8 BOM

    writeHeader(file, params);
    file << "\n";

    bool hasSideCylinder = !results.cylinder_stroke_full_side.empty();

    // Заголовок
    if (hasSideCylinder) {
        file << "alpha[град];stroke_full_main[м];velocity_full_main[м/с];acceleration_full_main[м/с²];"
             << "betta_rod_main[рад];omega_rod_main[рад/с];eps_rod_main[рад/с²];"
             << "stroke_full_side[м];velocity_full_side[м/с];acceleration_full_side[м/с²];"
             << "betta_rod_side[рад];omega_rod_side[рад/с];eps_rod_side[рад/с²]\n";
    } else {
        file << "alpha[град];stroke_full[м];velocity_full[м/с];acceleration_full[м/с²];"
             << "betta_rod[рад];omega_rod[рад/с];eps_rod[рад/с²]\n";
    }

    file << fixed << setprecision(6);
    size_t dataSize = results.alpha.size();

    for (size_t i = 0; i < dataSize; ++i) {
        file << results.alpha[i] << ";";

        // Основной цилиндр (индекс 0)
        if (!results.cylinder_stroke_full.empty() && i < results.cylinder_stroke_full[0].size()) {
            file << results.cylinder_stroke_full[0][i] << ";"
                 << results.cylinder_velocity_full[0][i] << ";"
                 << results.cylinder_acceleration_full[0][i] << ";"
                 << results.cylinder_betta_rod[0][i] << ";"
                 << results.cylinder_omega_rod[0][i] << ";"
                 << results.cylinder_eps_rod[0][i];
        } else {
            file << "0.0;0.0;0.0;0.0;0.0;0.0";
        }

        if (hasSideCylinder) {
            file << ";";
            if (!results.cylinder_stroke_full_side.empty() && i < results.cylinder_stroke_full_side[0].size()) {
                file << results.cylinder_stroke_full_side[0][i] << ";"
                     << results.cylinder_velocity_full_side[0][i] << ";"
                     << results.cylinder_acceleration_full_side[0][i] << ";"
                     << results.cylinder_betta_rod_side[0][i] << ";"
                     << results.cylinder_omega_rod_side[0][i] << ";"
                     << results.cylinder_eps_rod_side[0][i];
            } else {
                file << "0.0;0.0;0.0;0.0;0.0;0.0";
            }
        }
        file << "\n";
    }

    file.close();
    cout << "Сводные результаты сохранены в файл: " << outputFilename << endl;
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